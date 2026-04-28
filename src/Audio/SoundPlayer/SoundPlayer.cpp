#include "SoundPlayer.h"

#include "esp_log.h"
#include <freertos/FreeRTOS.h>

using namespace std;

size_t bytes_read;
size_t bytes_written;

#define AUDIO_BUFFER 2048 // buffer size for reading the wav file and sending to i2s
int16_t buffer[AUDIO_BUFFER];

SoundPlayer::SoundPlayer(I2SOutPins pinConfig, i2s_port_t i2sPort)
{
  stopped = true;
  i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(i2sPort, I2S_ROLE_MASTER);
  ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_handle, NULL));

  i2s_std_config_t std_cfg = {
      .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
      .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(i2s_data_bit_width_t(BIT_SAMPLE), i2s_slot_mode_t(NUM_CHANNELS)),
      .gpio_cfg = {
          .mclk = I2S_GPIO_UNUSED,
          .bclk = pinConfig.bck,
          .ws = pinConfig.ws,
          .dout = pinConfig.dout,
          .din = I2S_GPIO_UNUSED,
          .invert_flags = {
              .mclk_inv = false,
              .bclk_inv = false,
              .ws_inv = false}},
  };
  ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_handle, &std_cfg));
}

void SoundPlayer::moveToBeggin()
{
  fseek(currentPlaying, WAVE_HEADER_SIZE, SEEK_SET);
}

void SoundPlayer::playSound(string filePath, bool repeatSound)
{
  stopSound();
  repeat = repeatSound;
  currentPlaying = fopen(filePath.c_str(), "rb");
  if (currentPlaying == NULL)
  {
    ESP_LOGE(PLAYER_TAG, "Failed to open file %s", filePath.c_str());
    return;
  }

  moveToBeggin();
  ESP_ERROR_CHECK(i2s_channel_enable(tx_handle));
  stopped = false;
  ESP_LOGI(PLAYER_TAG, "Starting to play song file %s", filePath.c_str());
}

void SoundPlayer::playTone(int frequency, int amplitude)
{
  stopSound();
  this->halfWaveLength = (44100 / frequency) / 2;
  this->waveSample = amplitude;

  ESP_ERROR_CHECK(i2s_channel_enable(tx_handle));
  stopped = false;
  tonePlaying = true;
  ESP_LOGI(PLAYER_TAG, "Starting to play tone");
}

void SoundPlayer::stopSound()
{
  if (stopped)
  {
    return;
  }
  ESP_LOGI(PLAYER_TAG, "Stopping playing sound");
  stopped = true;
  ESP_ERROR_CHECK(i2s_channel_disable(tx_handle));
  tonePlaying = false;
  wavePhaseCount = 0;
  if (currentPlaying != NULL)
  {
    fclose(currentPlaying);
    currentPlaying = NULL;
  }
  ESP_LOGI(PLAYER_TAG, "Playing sound stopped");
}

bool SoundPlayer::isPlaying()
{
  return !stopped;
}

void SoundPlayer::loop()
{
  if (stopped)
  {
    return;
  }

  if (!tonePlaying)
  {
    if ((bytes_read = fread(buffer, sizeof(int16_t), AUDIO_BUFFER, currentPlaying)) > 0)
    {
      i2s_channel_write(tx_handle, buffer, bytes_read * sizeof(int16_t), &bytes_written, portMAX_DELAY);
    }
    else
    {
      if (repeat)
      {
        moveToBeggin();
        ESP_LOGI(PLAYER_TAG, "Repeating the sound");
        loop();
      }
      else
      {
        stopSound();
      }
    }
  }
  else
  {
    for (int i = 0; i < AUDIO_BUFFER; i++)
    {
      if (wavePhaseCount == halfWaveLength)
      {
        waveSample = -1 * waveSample;
        wavePhaseCount = 0;
      }
      else
      {
        wavePhaseCount++;
      }
      buffer[i] = waveSample;
    }

    i2s_channel_write(tx_handle, buffer, AUDIO_BUFFER, &bytes_written, portMAX_DELAY);
  }
}

SoundPlayer::~SoundPlayer()
{
  stopSound();
  ESP_ERROR_CHECK(i2s_del_channel(tx_handle));
}