#ifndef HEADER_AUDIO
#define HEADER_AUDIO

#include <hal/gpio_types.h>
#include <string>

#define WAVE_HEADER_SIZE 44
#define SAMPLE_RATE 16000
#define BIT_SAMPLE 32
#define NUM_CHANNELS 1
#define SAMPLE_SIZE 256
#define BYTE_RATE (SAMPLE_RATE * (BIT_SAMPLE / 8)) * NUM_CHANNELS

#endif