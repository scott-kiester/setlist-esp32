#include <memory>

#include <FS.h>

#include "audio/memwav.hpp"
#include "audio/player.hpp"
#include "audio.hpp"
#include "log.hpp"
#include "storage/sdcard-mem-fs.hpp"


#define I2S_DOUT   6 /* Data out */
#define I2S_BCLK  18 /* Clock */
#define I2S_WS     7 /* Word Select (LRC) */

// Flasher gets triggered from audio thread so they can stay in sync
#define FLASHER_PIN 47
#define FLASHER_DEFAULT_DELAY 1000
#define FLASHER_DEFAULT_FLASH_DURATION 25

#define TIME_TYPE unsigned long


// The message passing implementation here may be a bit naieve. Since there's
// only one task receiving messages and interacting with the audio object,
// it assumes that incoming threads will receive responses in the same order
// they were sent.
//
// While I think that's LIKELY to happen in all cases, I don't think it's 
// guaranteed currently. (What I'm really looking for here is semantics 
// similar to CK_SendMessage, but I don't think FreeRTOS has an exact
// equivilent. Bummer. Might have to roll my own.)


enum AudioMessage_t {
  AM_NoOp,
  AM_PlayFile,
  AM_SetClickFile,
  AM_StartClick,
  AM_PlayClick,
};


///////////////////////////////////////////////////////////////////////////////
// class AudioMessage
///////////////////////////////////////////////////////////////////////////////
class AudioMessage {
public:
  AudioMessage():
    data(NULL) {}

  AudioMessage(AudioMessage_t _message, void *_data):
    message(_message),
    data(_data) {}

  virtual ~AudioMessage() {}

  AudioMessage_t message;
  void *data;
};


template <class T> 
class AmSingleTypeMessage {
public:
  AmSingleTypeMessage(T _message): 
    message(_message) {}

  static T As(void *param) { return reinterpret_cast<AmSingleTypeMessage<T>*>(param)->message; }

  T message;
};


///////////////////////////////////////////////////////////////////////////////
// class Flasher
///////////////////////////////////////////////////////////////////////////////
class Flasher {
public:
  Flasher(): ledOnTime(0) {}
  virtual ~Flasher() {}

  void TurnOn();
  void TurnOffAfterDelay();

  bool LedIsOn();

  static void Init();

private:
  TIME_TYPE ledOnTime;
};


void Flasher::Init() {
  pinMode(FLASHER_PIN, OUTPUT);
}


bool Flasher::LedIsOn() { return ledOnTime != 0; }


void Flasher::TurnOn() {
  digitalWrite(FLASHER_PIN, HIGH);
  ledOnTime = millis();
}


void Flasher::TurnOffAfterDelay() {
  TIME_TYPE now = millis();
  if (ledOnTime != 0 && ledOnTime + FLASHER_DEFAULT_FLASH_DURATION < now) {
    digitalWrite(FLASHER_PIN, LOW);
    ledOnTime = 0;    
  }
}


///////////////////////////////////////////////////////////////////////////////
// class AudioPlayer
///////////////////////////////////////////////////////////////////////////////
class AudioPlayer {
public:
  AudioPlayer();
  virtual ~AudioPlayer();

  void Init();
  bool OnAudioPlayerThread();

  bool PlayAudioFile(const char *fileName);

  bool SetClickFile(const char *fileName);
  bool StartClick(uint16_t bpm);

  void StartClick() { clickOn = true; }
  void StopClick() { clickOn = false; }
  void StartFlash() { flashOn = true; }
  void StopFlash() { flashOn = false; }

private:
  static void audioPlayerTaskInit(void *param);
  void audioPlayerTask();

  AudioMessage_t waitForMessage(AudioMessage *inMessage);

  bool sendMessage(QueueHandle_t queue, AudioMessage *message);
  bool receiveMessageFromAudioThread(AudioMessage_t expectedResponseType, AudioMessage *message);

  void playAudioFile(const char *fileName);

  void setClickFile(const char *fileName);
  void startClick(uint16_t bpm);
  void playClick();

  TaskHandle_t audioTask;
  QueueHandle_t inMessages;
  QueueHandle_t outMessages;

  AudioLib::MemWav clickWav;
  bool playingClick;
  Flasher flasher;

  std::shared_ptr<fs::FS> clickFs;
  fs::FSImplPtr clickFsImpl;

  std::string clickFile;
  uint32_t clickDelay;
  uint32_t lastClickPlayed;

  bool clickOn;
  bool flashOn;
};


AudioPlayer::AudioPlayer():
  audioTask(NULL),
  inMessages(NULL),
  outMessages(NULL),
  playingClick(false),
  clickFsImpl(NULL),
  clickDelay(0),
  lastClickPlayed(0),
  clickOn(true),
  flashOn(true) {}


AudioPlayer::~AudioPlayer() {
  // Kill task and wait for it to exit? Why? If we ever get here then this code will look
  // different than it does now. 
  // 
  // I still feel the urge to clean stuff up here, anyway. Huh.

  if (inMessages) { vQueueDelete(inMessages); }
  if (outMessages) { vQueueDelete(outMessages); }
}


bool AudioPlayer::OnAudioPlayerThread() {
  return xTaskGetCurrentTaskHandle() == audioTask;
}


bool AudioPlayer::sendMessage(QueueHandle_t queue, AudioMessage *message) {
  bool success = false;

  BaseType_t ret = xQueueSend(queue, message, portMAX_DELAY);
  if (ret == pdTRUE) {
    success = true;
  } else {
    logPrintf(LOG_COMP_AUDIO, LOG_SEV_ERROR, "Unable to send message. AudioThread: %s, message: %d\n", 
      OnAudioPlayerThread() ? "True" : "False",
      message->message);
  }

  return success;
}


bool AudioPlayer::receiveMessageFromAudioThread(AudioMessage_t expectedResponseType, AudioMessage *message) {
  bool success = false;

  BaseType_t ret = xQueueReceive(outMessages, message, portMAX_DELAY);
  if (ret == pdPASS) {
    if (expectedResponseType == message->message) {
      success = true;
    } else {
      logPrintf(LOG_COMP_AUDIO, LOG_SEV_WARN, "receiveMessageFromAudioThread: Expected response: %d, but got: %d\n", 
        expectedResponseType, message->message);  
    }
  } else {
    logPrintf(LOG_COMP_AUDIO, LOG_SEV_ERROR, "receiveMessageFromAudioThread: Error reading response: %d\n", ret);
  }

  return success;
}


void AudioPlayer::Init() {
  Flasher::Init();

  inMessages = xQueueCreate(10, sizeof(AudioMessage));
  outMessages = xQueueCreate(10, sizeof(AudioMessage));

  BaseType_t ret = xTaskCreatePinnedToCore(
    AudioPlayer::audioPlayerTaskInit,
    "AudioPlayer",
    5000, 
    this,
    2 | portPRIVILEGE_BIT,
    &audioTask,
    1);

  if (ret == pdPASS) {
    logPrintf(LOG_COMP_AUDIO, LOG_SEV_INFO, "Audio: Created task: %d\n", audioTask);
  } else {
    logPrintf(LOG_COMP_AUDIO, LOG_SEV_ERROR, "***ERROR: Unable to create audio task: %d\n", ret);
  }
}


void AudioPlayer::audioPlayerTaskInit(void *param) {
  AudioPlayer *player = reinterpret_cast<AudioPlayer*>(param);
  player->audioPlayerTask();
}


AudioMessage_t AudioPlayer::waitForMessage(AudioMessage *inMessage) {
  if (clickDelay == 0) {
    if (xQueueReceive(inMessages, inMessage, 1) == pdPASS) {
      return inMessage->message;
    } else {
      return AM_NoOp;
    }
  }

  // If we're here, then we're playing the click sound. We want to see if there's
  // a message waiting, but we only want to wait until it's time to play the next
  // click.

  uint32_t curTime = millis();
  uint32_t playTime = lastClickPlayed + clickDelay;
  if (curTime < playTime) {
    TickType_t ticksToWait = 0;
    if (flasher.LedIsOn()) {
      // Wait until it's time to turn off the LED
      ticksToWait = FLASHER_DEFAULT_FLASH_DURATION * portTICK_PERIOD_MS;
    } else {
      // Wait until the next click
      ticksToWait = (playTime - curTime) * portTICK_PERIOD_MS; 
    }

    if (xQueueReceive(inMessages, inMessage, ticksToWait) == pdPASS) {
      // There's a message waiting. Process it.
      return inMessage->message;
    } else {
      // Timer expired without a message. Play the click.
      curTime = millis();
      if (curTime >= playTime) {
        return AM_PlayClick;
      } else {
        return AM_NoOp;
      }
    }
  }

  // It's probably slightly past time to play the click.
  return AM_PlayClick;
}


void AudioPlayer::audioPlayerTask() {
  AudioLib::Player::Init(I2S_BCLK, I2S_WS, I2S_DOUT);

  AudioMessage inMessage;

  while (true) {
    switch(waitForMessage(&inMessage)) {
      case AM_NoOp:
      // There were no messages in the queue. Allow the audio component to run, then
      // check again.
      break;

      case AM_PlayFile:
      playAudioFile(AmSingleTypeMessage<const char*>::As(inMessage.data));
      break;

      case AM_SetClickFile:
      setClickFile(AmSingleTypeMessage<const char*>::As(inMessage.data));
      break;

      case AM_StartClick:
      startClick(AmSingleTypeMessage<uint16_t>::As(inMessage.data));
      break;

      case AM_PlayClick:
      playClick();
      break;

      default:
        // Should not get here
        logPrintf(LOG_COMP_AUDIO, LOG_SEV_ERROR, "AUDIO: Unknown message: %d\n", inMessage.message);
    }

    flasher.TurnOffAfterDelay();
    AudioLib::Player::GetPlayer().WriteToDevice();
  }
}


void AudioPlayer::playClick() {
  lastClickPlayed = millis();

  if (flashOn) {
    flasher.TurnOn();
  }

  if (!clickOn || !clickWav.Valid()) {
    return;
  }

  // An assumption is made that we'll always get here after the previous click has stopped playing. (In other 
  // words, click sounds are expected to be short.) I should probably be ensuring that really is the case when 
  // we get here, though. It might also make sense to enforce a click time length limit in setClickFile().
  if (!playingClick) {
    if (AudioLib::Player::GetPlayer().Play(&clickWav)) {
      playingClick = true;
    }
  } else {
    AudioLib::Player::GetPlayer().Replay();
  }
}


void AudioPlayer::playAudioFile(const char *fileName) {
  //bool ret = audio.connecttoFS(SDCard::GetFS(), fileName);
  //bool ret = audio.connecttoSD(fileName);
  //logPrintf(LOG_COMP_AUDIO, LOG_SEV_INFO, "AUDIO: Start playing from FS. Name: %s, result: %s\n", fileName, ret ? "True" : "False");

  //AmSingleTypeMessage<bool> retMessage(ret);
  //AudioMessage audioMessage(AM_PlayFile, &retMessage);
  //sendMessage(outMessages, &audioMessage);
}


bool AudioPlayer::PlayAudioFile(const char *fileName) {
  bool success = false;

  AmSingleTypeMessage<const char*> message(fileName);
  AudioMessage send(AM_PlayFile, &message);
  if (sendMessage(inMessages, &send)) {
    logPrintf(LOG_COMP_AUDIO, LOG_SEV_VERBOSE, "AUDIO: Sent play message. Waiting for response...\n");

    AudioMessage response;
    if (receiveMessageFromAudioThread(AM_PlayFile, &response)) {
      logPrintf(LOG_COMP_AUDIO, LOG_SEV_VERBOSE, "AUDIO: Play message response received.\n");
      success = true;
    }
  }

  return success;
}


void AudioPlayer::setClickFile(const char *fileName) {
  try {
    logPrintf(LOG_COMP_AUDIO, LOG_SEV_VERBOSE, "AUDIO: Begin set click file name to %s\n", clickFile.c_str());

    bool success = clickWav.InitFromFile(fileName);
    if (success) {
      clickFile = fileName;
      logPrintf(LOG_COMP_AUDIO, LOG_SEV_INFO, "AUDIO: SUCCESS: Set click file name to %s\n", clickFile.c_str());
    } else {
      logPrintf(LOG_COMP_AUDIO, LOG_SEV_ERROR, "AUDIO: Failed to set click file to %s\n", fileName);  
    }

    AmSingleTypeMessage<bool> retMessage(success);
    AudioMessage audioMessage(AM_SetClickFile, &retMessage);
    sendMessage(outMessages, &audioMessage);

  } catch (std::bad_alloc&) {
    logPrintf(LOG_COMP_AUDIO, LOG_SEV_ERROR, "AUDIO: Unable to allocate space for click file name.\n");
  }
}


bool AudioPlayer::SetClickFile(const char *fileName) {
  bool success = false;

  AmSingleTypeMessage<const char*> message(fileName);
  AudioMessage send(AM_SetClickFile, &message);
  if (sendMessage(inMessages, &send)) {
    logPrintf(LOG_COMP_AUDIO, LOG_SEV_VERBOSE, "AUDIO: Sent SetClickFile message. Waiting for response...\n");

    AudioMessage response;
    if (receiveMessageFromAudioThread(AM_SetClickFile, &response)) {
      logPrintf(LOG_COMP_AUDIO, LOG_SEV_VERBOSE, "AUDIO: SetClickFile message response received.\n");
      success = true;
    }
  }

  return success;
}


bool AudioPlayer::StartClick(uint16_t bpm) {
  bool success = false;

  AmSingleTypeMessage<uint16_t> message(bpm);
  AudioMessage send(AM_StartClick, &message);
  if (sendMessage(inMessages, &send)) {
    logPrintf(LOG_COMP_AUDIO, LOG_SEV_VERBOSE, "AUDIO: Sent StartClick message. Waiting for response...\n");

    AudioMessage response;
    if (receiveMessageFromAudioThread(AM_StartClick, &response)) {
      logPrintf(LOG_COMP_AUDIO, LOG_SEV_VERBOSE, "AUDIO: StartClick message response received.\n");
      success = true;
    }
  }

  return success;
}


void AudioPlayer::startClick(uint16_t bpm) {
  // Figure out the number of milliseconds between flashes
  clickDelay = (((float)60) / bpm) * 1000;
  lastClickPlayed = 0;

  AmSingleTypeMessage<bool> retMessage(true);
  AudioMessage audioMessage(AM_StartClick, &retMessage);
  sendMessage(outMessages, &audioMessage);
}



AudioPlayer audioPlayer;


///////////////////////////////////////////////////////////////////////////////
// namespace AudioComp
///////////////////////////////////////////////////////////////////////////////

void AudioComp::Init() {
  audioPlayer.Init();
}


bool AudioComp::PlayAudioFile(const char *fileName) {
  return audioPlayer.PlayAudioFile(fileName);
}


bool AudioComp::SetClickFile(const char *fileName) {
  return audioPlayer.SetClickFile(fileName);
}


bool AudioComp::StartClick(uint16_t bpm) {
  return audioPlayer.StartClick(bpm);
}


bool AudioComp::StartClick() {
  audioPlayer.StartClick();
  return true;
}


bool AudioComp::StopClick() {
  audioPlayer.StopClick();
  return true;
}


bool AudioComp::StartFlash() {
  audioPlayer.StartFlash();
  return true;
}


bool AudioComp::StopFlash() {
  audioPlayer.StopFlash();
  return true;
}
