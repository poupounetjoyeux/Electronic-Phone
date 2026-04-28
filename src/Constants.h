#ifndef HEADER_PINS
#define HEADER_PINS

#include "Audio/SoundPlayer/SoundPlayer.h"
#include "Audio/SoundRecorder/SoundRecorder.h"
#include "SD/SD.h"
#include "NfcReader/NfcReader.h"
#include "MqttClient/MqttClient.h"
#include "Wifi/Wifi.h"

#define TAG "[Phone]"

// GPIO

gpio_num_t phonePin = gpio_num_t(25);

// NFC

NfcPins nfcPins = {
    .sck = gpio_num_t(18),
    .mosi = gpio_num_t(23),
    .miso = gpio_num_t(19),
    .ss = gpio_num_t(5)
};

// SD ports

#define MOUNT_POINT "/sdcard"


SDCardPins sdPins = {
    .miso = gpio_num_t(13),
    .mosi = gpio_num_t(14),
    .sclk = gpio_num_t(26),
    .cs = gpio_num_t(27),
};

// I2S speaker

const I2SOutPins speakerPins = {
    .bck = gpio_num_t(33),
    .ws = gpio_num_t(32),
    .dout = gpio_num_t(22)};

// I2S microphone

const I2SInPins microphonePins = {
    .bck = gpio_num_t(17),
    .ws = gpio_num_t(16),
    .din = gpio_num_t(21)};

// Wifi

const WifiClientOptions wifiOptions = {
    .ssid = "Mariage",
    .pass = "MorganeQuentin73"
};

// MQTT

const MqttClientOptions mqttOptions = {
    .broker = "mqtt://192.168.1.27",
    .clientId = "Phone"
};

#define DEVICE_NAME "Phone"
#define STATS_TOPIC DEVICE_NAME"/stats"

#endif
