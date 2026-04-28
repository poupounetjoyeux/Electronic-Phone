#include "Constants.h"
#include "esp_log.h"

extern "C"
{
  void app_main(void);
}

SDCard sd(sdPins, MOUNT_POINT);
NfcReader nfc(nfcPins);
SoundPlayer player(speakerPins, I2S_NUM_0);
SoundRecorder recorder(microphonePins, I2S_NUM_1);
WifiClient wifi(wifiOptions);
MqttClient mqttClient(mqttOptions);
int currentFileNumber = 0;
bool recordPhase = false;
bool readPhase = false;

uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0};
uint8_t uidLength;

bool loadLastFileNumber()
{
  FILE *numberFile = fopen(MOUNT_POINT "/count", "ab+");
  if (numberFile == NULL)
  {
    ESP_LOGE(TAG, "Unable to open count file");
    return false;
  }

  fseek(numberFile, 0, SEEK_SET);
  if (!fread(&currentFileNumber, sizeof(int), 1, numberFile))
  {
    currentFileNumber = 0;
  }
  else
  {
    currentFileNumber++;
  }
  fclose(numberFile);
  return true;
}

bool setup()
{
  ESP_LOGI(TAG, "Setupping GPIO");
  ESP_ERROR_CHECK(gpio_set_direction(phonePin, GPIO_MODE_INPUT));
  ESP_ERROR_CHECK(gpio_set_pull_mode(phonePin, GPIO_PULLUP_ONLY));
  ESP_LOGI(TAG, "GPIO setupped");

  ESP_LOGI(TAG, "Setupping SD");
  if (!sd.init())
  {
    ESP_LOGE(TAG, "Unable to init SD");
    return false;
  }
  ESP_LOGI(TAG, "SD setupped");

  ESP_LOGI(TAG, "Loading last number from SD");
  if (!loadLastFileNumber())
  {
    ESP_LOGE(TAG, "Unable to load last number from SD");
    return false;
  }
  ESP_LOGI(TAG, "File will start with number : %d", currentFileNumber);

  ESP_LOGI(TAG, "Setupping NFC");
  if (!nfc.init())
  {
    ESP_LOGE(TAG, "Unable to init NFC");
    return false;
  }
  ESP_LOGI(TAG, "NFC setupped");

  ESP_LOGI(TAG, "Setupping Wifi");
  if(!wifi.init())
  {
    ESP_LOGE(TAG, "Unable to init Wifi");
    return false;
  }
  ESP_LOGI(TAG, "Wifi setupped");

  ESP_LOGI(TAG, "Setupping MQTT");
  if(!mqttClient.init())
  {
    ESP_LOGE(TAG, "Unable to init MQTT");
    return false;
  }
  ESP_LOGI(TAG, "MQTT setupped");

  return true;
}

bool checkForUid()
{
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength))
  {
    ESP_LOGI(TAG, "UID Value:");
    esp_log_buffer_hexdump_internal(TAG, uid, uidLength, ESP_LOG_INFO);
    return true;
  }
  return false;
}

bool isPhoneClosed()
{
  return gpio_get_level(phonePin) == 1;
}

void stopRecordingAndWriteCounter()
{
  recorder.stopRecording();
  FILE *numberFile = fopen(MOUNT_POINT "/count", "wb");
  if (numberFile == NULL)
  {
    ESP_LOGE(TAG, "Unable to open count file");
    return;
  }
  fwrite(&currentFileNumber, sizeof(int), 1, numberFile);
  fclose(numberFile);
  char payload[255];
  sprintf(payload, 
    "{\"StatType\":\"Message\",\"StatValues\":{\"AuthorNfcUid\":\"%02x%02x%02x%02x%02x%02x%02x\",\"FileName\":\"%.5d.wav\"}}", 
    uid[0],
    uid[1],
    uid[2],
    uid[3],
    uid[4],
    uid[5],
    uid[6],
    currentFileNumber);
  mqttClient.publish(STATS_TOPIC, payload, false);
  currentFileNumber++;
  ESP_LOGI(TAG, "Counter is now at %d", currentFileNumber);
  recordPhase = false;
}

void app_main(void)
{
  ESP_LOGI(TAG, "Starting");

  if (!setup())
  {
    ESP_LOGI(TAG, "Setup failed");
    return;
  }

  while (true)
  {
    if (isPhoneClosed())
    {
      recordPhase = false;
      readPhase = false;
      if (recorder.isRecording())
      {
        stopRecordingAndWriteCounter();
      }
      player.stopSound();
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    else
    {
      if (recorder.isRecording())
      {
        // max 2mins
        if (recorder.getRecordingTime() > 120)
        {
          stopRecordingAndWriteCounter();
          player.playSound(MOUNT_POINT "/END.WAV");
        }
        else
        {
          recorder.loop();
        }
      }
      else
      {
        if (recordPhase)
        {
          if (!player.isPlaying())
          {
            char recordName[64];
            sprintf(recordName, MOUNT_POINT "/%.5d.wav", currentFileNumber);
            recorder.startRecording(recordName);
          }
        }
        else if (!player.isPlaying())
        {
          player.playTone(440, 500);
          readPhase = true;
        }
        else if (readPhase && checkForUid())
        {
          player.stopSound();
          ESP_LOGI(TAG, "Hey hey tag reads!");
          recordPhase = true;
          readPhase = false;

          player.playSound(MOUNT_POINT "/RESP.WAV");
        }
        player.loop();
      }
    }
  }
}