#include <driver/adc.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <audio.hpp>
#include "components/component.hpp"
#include "log.hpp"
#include "trigger.hpp"


// Task names may not be longer than 16 chars in FreeRTOS
#define TRIGGER_PROCESSOR_THREAD_NAME "TriggerProc"
#define TRIGGER_LISTENER_THREAD_NAME "TriggerListnr"

// Number of conversions that can be performed on a single interrupt
#define CONV_NUM_PER_INTERRUPT 64
#define CONV_STORE_BUF_SIZE 4096

#define POLL_MS 250

// Pin 5, which is GPIO 5, which is ADC1 channel 4
//#define METRONOME_CHANNEL ADC1_CHANNEL_4

// Pin 4, which is GPIO 4, which is ADC1 channel 3
#define METRONOME_CHANNEL ADC1_CHANNEL_3


#define TRIGGER_THRESHOLD 4000
uint32_t maxTriggerVal = TRIGGER_THRESHOLD;
esp_err_t errVal = ESP_OK;

// In an effort to avoid false positives on fast reads, two positive readings within this amount of time 
// will be considered a single reading.
#define DELAY_TIME_BETWEEN_SEPARATE_READINGS 100

#define TRIGGER_EVENT_TRIGGERED 0x00000001

uint32_t clickRestartTime = 0;

void triggerProcessor(void *) {
  while (true) {
    if (errVal != ESP_OK) {
      esp_err_t printErrVal = errVal;
      errVal = ESP_OK;
      logPrintf(LOG_COMP_TRIGGER, LOG_SEV_ERROR, "Got error %d from adc_digi_read_bytes while reading triggers.\n", printErrVal);
    }

    // Wait for a trigger event.
    if (!ulTaskNotifyTake(pdTRUE, portMAX_DELAY)) {
      continue;
    }

    if (maxTriggerVal > TRIGGER_THRESHOLD) {
      TIME_TYPE now = millis();
      static TIME_TYPE lastPositiveReadTime = 0;
      if (now - lastPositiveReadTime > DELAY_TIME_BETWEEN_SEPARATE_READINGS) {
        // Restart the click *NOW*
        AudioComp::RestartClick(clickRestartTime);

        lastPositiveReadTime = now;
        uint32_t triggerVal = maxTriggerVal;
        maxTriggerVal = TRIGGER_THRESHOLD;
        logPrintf(LOG_COMP_TRIGGER, LOG_SEV_VERBOSE, "THWACK!!! %d\n", triggerVal);
      } else {
        maxTriggerVal = TRIGGER_THRESHOLD;
      }
    }

    delay(10);
  }
}


void listenForTriggers(void *param) {
  logPrintf(LOG_COMP_TRIGGER, LOG_SEV_INFO, "Trigger Listener initializing...\n");

  TaskHandle_t triggerProcessorHandle = param;

  adc_digi_init_config_t adcDmaConfig = {
    .max_store_buf_size = CONV_STORE_BUF_SIZE,
    .conv_num_each_intr = CONV_NUM_PER_INTERRUPT,
    .adc1_chan_mask = METRONOME_CHANNEL,
    .adc2_chan_mask = 0,
  };

  esp_err_t err = adc_digi_initialize(&adcDmaConfig);
  if (err != ESP_OK) {
    logPrintf(LOG_COMP_TRIGGER, LOG_SEV_ERROR, "Error from adc_digi_initialize: %d\n", err);
    return;
  }

  adc_digi_pattern_config_t digiPattern = {
    .atten = ADC_ATTEN_DB_0,
    .channel = METRONOME_CHANNEL,
    .unit = 0, // ADC1
    .bit_width = SOC_ADC_DIGI_MAX_BITWIDTH,
  };

  adc_digi_configuration_t digiConf = {
    .conv_limit_en = 0,
    .conv_limit_num = 250,
    .pattern_num = 1,
    .adc_pattern = &digiPattern,
    .sample_freq_hz = 1000,
    .conv_mode = ADC_CONV_SINGLE_UNIT_1,
    .format = ADC_DIGI_OUTPUT_FORMAT_TYPE2,
  };

  err = adc_digi_controller_configure(&digiConf);
  if (err != ESP_OK) {
    logPrintf(LOG_COMP_TRIGGER, LOG_SEV_ERROR, "Error from adc_digi_controller_configure: %d\n", err);
    return;
  }

  err = adc_digi_start();
  if (err != ESP_OK) {
    logPrintf(LOG_COMP_TRIGGER, LOG_SEV_ERROR, "Error from adc_digi_start: %d\n", err);
    return;
  }

  logPrintf(LOG_COMP_TRIGGER, LOG_SEV_INFO, "Trigger Listener started.\n");

  TIME_TYPE lastInvalidStateTime = 0;

  uint8_t results[CONV_NUM_PER_INTERRUPT];
  while (true) {
    uint32_t numResults = 0;
    err = adc_digi_read_bytes(results, CONV_NUM_PER_INTERRUPT, &numResults, ADC_MAX_DELAY);//POLL_MS);
    if (err == ESP_ERR_INVALID_STATE) {
      TIME_TYPE now = millis();

      // Avoid spamming the output with warnings.
      if (now - lastInvalidStateTime > 30 * 1000) {
        // We're not reading fast enough, so likely missed something. 
        logPrintf(LOG_COMP_TRIGGER, LOG_SEV_WARN, "Got ESP_ERR_INVALID_STATE from adc_digi_read_bytes while reading trigger input. We're not doing reads fast enough.\n");
        lastInvalidStateTime = millis();
      }

      // Go ahead and process what we have
      err = ESP_OK;
    }

    if (err == ESP_OK) {
      // Process the data and see if there's a hit.
      for (uint32_t i = 0; i < numResults; i++) {
        adc_digi_output_data_t *data = reinterpret_cast<adc_digi_output_data_t*>(&(results[i]));
        if (data->type2.unit == 0) {
          uint32_t dataVal = data->type2.data;
          if (dataVal > maxTriggerVal) {
            maxTriggerVal = dataVal;
            // If we get here, we have a valid trigger and the processor hasn't picked it up yet.
            // (It could be the first one.)
            if (dataVal > TRIGGER_THRESHOLD) {
              // Timing is critical here, as we want the click to start RIGHT when the trigger is hit. We can't
              // pass a message to the audio thread quickly enough, so instead we'll tell it when the click was
              // supposed to start, and subsequent clicks will be accurate.
              clickRestartTime = millis();
              xTaskNotifyGive(triggerProcessorHandle);
              break; // No point in continuing to process stuff
            }
          }
        }
      }
    } else if (err == ESP_ERR_TIMEOUT) {
      // If the conversion is not finished by POLL_MS, we'll get here. In that case we might need to shrink the
      // number of samples read per interrupt (I think).
      logPrintf(LOG_COMP_TRIGGER, LOG_SEV_WARN, "Got ESP_ERR_TIMEOUT from adc_digi_read_bytes while reading trigger input. Might need to change some parameters.\n");
    } else {
      errVal = err;
    }

    yield();
  }
}


void Trigger::Init() {
  TaskHandle_t processorTask = NULL;

  // Processing happens on a dedicated thread, to avoid slowing down the frequency at which we call adc_digi_read_bytes.
  xTaskCreate(&triggerProcessor, TRIGGER_PROCESSOR_THREAD_NAME, 1024 * 2, NULL, 5, &processorTask);
  xTaskCreate(&listenForTriggers, TRIGGER_LISTENER_THREAD_NAME, 1024 * 4, processorTask, 5, NULL);

  logPrintf(LOG_COMP_TRIGGER, LOG_SEV_VERBOSE, "Done initializing trigger module.\n");
}
