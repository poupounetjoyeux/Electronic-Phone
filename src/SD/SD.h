#ifndef HEADER_SD
#define HEADER_SD

#include <hal/gpio_types.h>
#include <string>
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"

#define SD_TAG "[SDCard]"

struct SDCardPins
{
    gpio_num_t miso;
    gpio_num_t mosi;
    gpio_num_t sclk;
    gpio_num_t cs;
};

class SDCard
{
public:
    SDCard(SDCardPins pinsConfiguration, const char* mountPoint);
    ~SDCard();
    bool init(bool format = false);

private:
    SDCardPins pinsConfiguration;
    const char *mountPoint;
    sdmmc_card_t *card;
    spi_host_device_t spiDeviceSlot;
    bool isInit;
};

#endif