#ifndef HEADER_SOUND_PLAYER
#define HEADER_SOUND_PLAYER

#include "../Audio.h"
#include "driver/i2s_std.h"

#define PLAYER_TAG "[SoundPlayer]"

struct I2SOutPins
{
    gpio_num_t bck;
    gpio_num_t ws;
    gpio_num_t dout;
};

class SoundPlayer
{
public:
    SoundPlayer(I2SOutPins pinConfig, i2s_port_t i2sPort);
    ~SoundPlayer();
    void playSound(std::string filePath, bool repeatSound = false);
    void playTone(int frequency, int amplitude);
    void stopSound();
    bool isPlaying();
    void loop();

private:
    void moveToBeggin();
    i2s_chan_handle_t tx_handle;
    i2s_port_t i2sPort;
    FILE *currentPlaying;
    bool stopped;
    bool repeat;

    int sampleRate;
    int halfWaveLength;
    int wavePhaseCount;
    short waveSample;
    bool tonePlaying;
};

#endif