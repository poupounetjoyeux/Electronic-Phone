#ifndef HEADER_SOUND_RECORDER
#define HEADER_SOUND_RECORDER

#include "../Audio.h"
#include "driver/i2s_std.h"

#define RECORDER_TAG "[SoundRecorder]"

#define BYTE_RATE (SAMPLE_RATE * (I2S_DATA_BIT_WIDTH_32BIT / 8)) * NUM_CHANNELS

struct I2SInPins
{
    gpio_num_t bck;
    gpio_num_t ws;
    gpio_num_t din;
};

class SoundRecorder
{
public:
    SoundRecorder(I2SInPins pinConfig, i2s_port_t i2sPort);
    ~SoundRecorder();
    void init();
    void startRecording(std::string filePath);
    void stopRecording();
    bool isRecording();
    uint32_t getRecordingTime();
    void loop();

private:
    void writeWavHeader();
    I2SInPins pinConfig;
    i2s_chan_handle_t rx_handle;
    i2s_port_t i2sPort;
    FILE *currentRecording;
    uint32_t currentSize;
    bool stopped;
    bool isInit;
};

typedef struct
{
    struct
    {
        char chunk_id[4];     /*!< Contains the letters "RIFF" in ASCII form */
        uint32_t chunk_size;  /*!< This is the size of the rest of the chunk following this number */
        char chunk_format[4]; /*!< Contains the letters "WAVE" */
    } descriptor_chunk;       /*!< Canonical WAVE format starts with the RIFF header */
    struct
    {
        char subchunk_id[4];      /*!< Contains the letters "fmt " */
        uint32_t subchunk_size;   /*!< This is the size of the rest of the Subchunk which follows this number */
        uint16_t audio_format;    /*!< PCM = 1, values other than 1 indicate some form of compression */
        uint16_t num_of_channels; /*!< Mono = 1, Stereo = 2, etc. */
        uint32_t sample_rate;     /*!< 8000, 44100, etc. */
        uint32_t byte_rate;       /*!< ==SampleRate * NumChannels * BitsPerSample s/ 8 */
        uint16_t block_align;     /*!< ==NumChannels * BitsPerSample / 8 */
        uint16_t bits_per_sample; /*!< 8 bits = 8, 16 bits = 16, etc. */
    } fmt_chunk;                  /*!< The "fmt " subchunk describes the sound data's format */
    struct
    {
        char subchunk_id[4];    /*!< Contains the letters "data" */
        uint32_t subchunk_size; /*!< ==NumSamples * NumChannels * BitsPerSample / 8 */
        int16_t data[0];        /*!< Holds raw audio data */
    } data_chunk;               /*!< The "data" subchunk contains the size of the data and the actual sound */
} wav_header_t;

#define WAV_HEADER_PCM_DEFAULT(sampleSize)                                                       \
    {                                                                                            \
        .descriptor_chunk = {                                                                    \
            .chunk_id = {'R', 'I', 'F', 'F'},                                                    \
            .chunk_size = (sampleSize) + sizeof(wav_header_t) - 8,                               \
            .chunk_format = {'W', 'A', 'V', 'E'}},                                               \
        .fmt_chunk = {.subchunk_id = {'f', 'm', 't', ' '}, .subchunk_size = 16, /* 16 for PCM */ \
                      .audio_format = 1,                                        /* 1 for PCM */  \
                      .num_of_channels = (NUM_CHANNELS),                                         \
                      .sample_rate = (SAMPLE_RATE),                                              \
                      .byte_rate = (I2S_DATA_BIT_WIDTH_32BIT) * (SAMPLE_RATE) * (NUM_CHANNELS) / 8,            \
                      .block_align = (I2S_DATA_BIT_WIDTH_32BIT) * (NUM_CHANNELS) / 8,                          \
                      .bits_per_sample = (I2S_DATA_BIT_WIDTH_32BIT)},                                          \
        .data_chunk = {                                                                          \
            .subchunk_id = {'d', 'a', 't', 'a'},                                                 \
            .subchunk_size = (sampleSize)                                                        \
        }                                                                                        \
    }

#endif