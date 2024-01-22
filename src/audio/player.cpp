#include <driver/i2s.h>

#include "audio/player.hpp"
#include "log.hpp"


namespace AudioLib {


Player::Player():
  playThis(NULL),
  processedSamplesLen(0),
  processedSamplesTotalLen(0),
  playing(false),
  samplesIdx(0),
  volume(0.1) {}


Player::~Player() {}


bool Player::Play(AudioDataInterface* _playThis) {
  logPrintf(LOG_COMP_AUDIO, LOG_SEV_ERROR, "Setting sample rate to: %d\n", _playThis->GetSampleRate());
  /*
  esp_err_t err = i2s_set_sample_rates(I2S_NUM_0, _playThis->GetSampleRate());
  if (err != ESP_OK) {
    logPrintf(LOG_COMP_AUDIO, LOG_SEV_ERROR, "Error from i2s_set_sample_rates: %d\n", err);
    return false;
  }
  */

  esp_err_t err = i2s_set_clk(I2S_NUM_0, _playThis->GetSampleRate(), _playThis->GetBitsPerSample(), I2S_CHANNEL_STEREO);
  if (err != ESP_OK) {
    logPrintf(LOG_COMP_AUDIO, LOG_SEV_ERROR, "Error from i2s_set_sample_rates: %d\n", err);
    return false;
  }

  playThis = _playThis;

  playing = true;
  return true;
}


bool Player::Replay() {
  if (!playThis) {
    logPrintf(LOG_COMP_AUDIO, LOG_SEV_ERROR, "Error: You must call Play() before you can call Replay()\n");
    return false;
  }

  samplesIdx = 0;
  playing = true;
  return true;
}


bool Player::WriteToDevice() {
  yield();

  if (!playing) {
    return true;
  }

  if (!playThis) {
    // If we get here, it's a bug
    logPrintf(LOG_COMP_AUDIO, LOG_SEV_ERROR, "*** ERROR: Playing audio: playing == true, but playThis == NULL\n");
    return false;
  }

  if (samplesIdx == 0) {
    samples = playThis->GetSamples();
    if (!samples) {
      // Data is likely invalid
      return false;
    }

    if (samplesIdx == samples->len) {
      if (!playThis->HasMoreData()) {
        // Time to get some more data
        // TODO: Implement!
      } else {
        // Nothing to do. We've played the entire set of samples.
        return true;
      }
    }

    if (samplesIdx > samples->len) {
      // If we get here, it's a bug
      logPrintf(LOG_COMP_AUDIO, LOG_SEV_ERROR, "*** ERROR: Playing audio: samplesIdx(%d) > samples.len(%d)\n", samplesIdx, samples->len);
      return false;
    }

    if (!processSamples()) {
      logPrintf(LOG_COMP_AUDIO, LOG_SEV_ERROR, "*** ERROR: Failed processing samples\n");
      return false;
    }
  }

  size_t written = 0;
  esp_err_t err = i2s_write(I2S_NUM_0, processedSamples.get() + samplesIdx, processedSamplesLen - samplesIdx, &written, 1);
  if (err != ESP_OK) {
    logPrintf(LOG_COMP_AUDIO, LOG_SEV_ERROR, "*** Error from i2s_write: %d\n", err);
    playing = false;
    samplesIdx = 0;
    return false;
  }

  samplesIdx += written;
  if (samplesIdx >= samples->len && !playThis->HasMoreData()) {
    // We've played the whole thing
    playing = false; // *** Is this correct for the MP3 case? Seems wrong...
    samplesIdx = 0;
  }

  return true;
}


bool Player::processSamples() {
  // Apply effects processing. Right now this is just a volume adjustment (and 
  // it might stay that way).

  static float volumeChange = 0.05;
  if (volume >= 1 || volume <= 0) {
    volumeChange = -volumeChange;
  }

  volume += volumeChange;

  try {

    // Allocate a buffer for the processed samples, as we don't want to modify the
    // buffer that was passed to us. For things like MemWav, that would change the
    // original data and would be reflected on the next playthrough.
    if (processedSamplesTotalLen < samples->len) {
      uint32_t newLen = 0;
      while (newLen < samples->len) {
        newLen += 1024;
      }

      logPrintf(LOG_COMP_AUDIO, LOG_SEV_VERBOSE, "Allocating new processed samples buffer of length %d\n", newLen);

      processedSamples = std::unique_ptr<uint8_t[]>(new uint8_t[newLen]);
      if (!processedSamples) {
        logPrintf(LOG_COMP_AUDIO, LOG_SEV_ERROR, "Failed to allocate buffer of length %d for processed samples\n", newLen);
        return false;
      }

      processedSamplesTotalLen = newLen;
    }

    // Consider using a different algorithm here where we shift bits, rather than
    // doing floating-point math.
    const int16_t *samples16 = reinterpret_cast<const int16_t*>(samples->samples);
    uint32_t samples16Len = samples->len / 2;

    int16_t *processedSamples16 = reinterpret_cast<int16_t*>(processedSamples.get());

    //logPrintf(LOG_COMP_AUDIO, LOG_SEV_VERBOSE, "Applying volume of %s\n", std::to_string(volume).c_str());

    for (uint16_t i = 0; i < samples16Len; i++) {
      processedSamples16[i] = samples16[i] * volume; 
    }

    processedSamplesLen = samples->len;

  } catch (std::bad_alloc&) {
    logPrintf(LOG_COMP_AUDIO, LOG_SEV_ERROR, "bad_alloc exception while processing samples\n");
    processedSamplesLen = processedSamplesTotalLen = 0;
    return false;
  }

  //logPrintf(LOG_COMP_AUDIO, LOG_SEV_VERBOSE, "Done applying volume\n");

  return true;
}


void Player::Init(uint8_t bckPin, uint8_t wsPin, uint8_t dataOutPin) {
  // Install the I2S driver
  // Note: ESP32-S3 does not have a built-in DAC

  i2s_config_t i2sConfig; 
  i2sConfig.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_TX);
  i2sConfig.sample_rate = 44100;
  i2sConfig.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  i2sConfig.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
  i2sConfig.communication_format = static_cast<i2s_comm_format_t>(I2S_COMM_FORMAT_STAND_I2S);
  i2sConfig.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1; // High interrupt priority
  i2sConfig.dma_buf_count = 8; // Max buffers
  i2sConfig.dma_buf_len = 1024; // Max value
  i2sConfig.use_apll = 0; // must be disabled in V2.0.1-RC1
  i2sConfig.tx_desc_auto_clear = true;
  i2sConfig.fixed_mclk = 0;
  i2sConfig.mclk_multiple = I2S_MCLK_MULTIPLE_DEFAULT;
  i2sConfig.bits_per_chan = I2S_BITS_PER_CHAN_DEFAULT;

  esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2sConfig, 0, NULL);
  if (err != ESP_OK) {
    logPrintf(LOG_COMP_AUDIO, LOG_SEV_ERROR, "Error from i2s_driver_install: %d\n", err);
    return;
  }

  i2s_zero_dma_buffer(I2S_NUM_0);

  i2s_pin_config_t pinConfig;
  pinConfig.bck_io_num = bckPin;
  pinConfig.ws_io_num = wsPin;
  pinConfig.data_out_num = dataOutPin;
  pinConfig.data_in_num = I2S_PIN_NO_CHANGE;

  err = i2s_set_pin(I2S_NUM_0, &pinConfig);
  if (err != ESP_OK) {
    logPrintf(LOG_COMP_AUDIO, LOG_SEV_ERROR, "Error from i2s_set_pin: %d\n", err);
    return;
  }

  logPrintf(LOG_COMP_AUDIO, LOG_SEV_VERBOSE, "I2S driver installed successfully\n");
}


Player& Player::GetPlayer() {
  static Player player;
  return player;
}


} // namespace AudioLib
