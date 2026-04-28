#include "SoundRecorder.h"

#include "esp_log.h"
#include <sys/stat.h>
#include <sys/unistd.h>

using namespace std;

SoundRecorder::SoundRecorder(I2SInPins pinConfig, i2s_port_t i2sPort)
{
    stopped = true;
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(i2sPort, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_handle));

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(i2s_data_bit_width_t(BIT_SAMPLE), i2s_slot_mode_t(NUM_CHANNELS)),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = pinConfig.bck,
            .ws = pinConfig.ws,
            .dout = I2S_GPIO_UNUSED,
            .din = pinConfig.din,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = true,
                .ws_inv = false}}};
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle, &std_cfg));
}

void SoundRecorder::writeWavHeader()
{
    fseek(currentRecording, 0, SEEK_SET);
    wav_header_t waveHeader = WAV_HEADER_PCM_DEFAULT(currentSize);
    fwrite(&waveHeader, sizeof(wav_header_t), 1, currentRecording);
}

void SoundRecorder::startRecording(std::string filePath)
{
    currentSize = 0;
    ESP_LOGI(RECORDER_TAG, "Opening file %s for recording", filePath.c_str());

    errno = 0;
    currentRecording = fopen(filePath.c_str(), "w");
    if (currentRecording == NULL)
    {
        ESP_LOGE(RECORDER_TAG, "Failed to open file for recording %s with error %d", filePath.c_str(), errno);
        return;
    }

    char wav_header_fmt[WAVE_HEADER_SIZE] = {0};
    fwrite(wav_header_fmt, 1, WAVE_HEADER_SIZE, currentRecording);
    ESP_ERROR_CHECK(i2s_channel_enable(rx_handle));
    stopped = false;
    ESP_LOGI(RECORDER_TAG, "Starting to record song file %s", filePath.c_str());
}

void SoundRecorder::stopRecording()
{
    if (stopped)
    {
        return;
    }
    ESP_LOGI(RECORDER_TAG, "Stopping recording sound");
    stopped = true;
    writeWavHeader();
    fclose(currentRecording);
    ESP_ERROR_CHECK(i2s_channel_disable(rx_handle));
    ESP_LOGI(RECORDER_TAG, "Recording sound stopped after %lu seconds", getRecordingTime());
}

bool SoundRecorder::isRecording()
{
    return !stopped;
}

uint32_t SoundRecorder::getRecordingTime()
{
    return currentSize / BYTE_RATE;
}

void SoundRecorder::loop()
{
    if (stopped)
    {
        return;
    }

    int16_t buffer[SAMPLE_SIZE];
    size_t bytesRead;
    if (i2s_channel_read(rx_handle, buffer, SAMPLE_SIZE, &bytesRead, 1000) == ESP_OK)
    {
        fwrite(buffer, 1, bytesRead, currentRecording);
        currentSize += bytesRead;
    }
    else
    {
        ESP_LOGE(RECORDER_TAG, "Error reading microphone block");
    }
}

SoundRecorder::~SoundRecorder()
{
    stopRecording();
    ESP_ERROR_CHECK(i2s_del_channel(rx_handle));
}