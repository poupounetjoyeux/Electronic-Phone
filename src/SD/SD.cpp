#include "SD.h"
#include "esp_log.h"
#include <sys/stat.h>

using namespace std;

SDCard::SDCard(SDCardPins pinsConfiguration, const char* mountPoint)
{
    isInit = false;
    this->pinsConfiguration = pinsConfiguration;
    this->mountPoint = mountPoint;
}

bool SDCard::init(bool format)
{
    if (isInit)
    {
        ESP_LOGE(SD_TAG, "We cannot init an SD card that is already init");
        return false;
    }

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = format,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024};

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = pinsConfiguration.mosi,
        .miso_io_num = pinsConfiguration.miso,
        .sclk_io_num = pinsConfiguration.sclk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    spiDeviceSlot = spi_host_device_t(host.slot);
    ESP_ERROR_CHECK(spi_bus_initialize(spiDeviceSlot, &bus_cfg, SDSPI_DEFAULT_DMA));

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = pinsConfiguration.cs;
    slot_config.host_id = spiDeviceSlot;

    ESP_LOGI(SD_TAG, "Mounting filesystem on %s", mountPoint);
    vTaskDelay(1000/portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(esp_vfs_fat_sdspi_mount(mountPoint, &host, &slot_config, &mount_config, &card));
    ESP_LOGI(SD_TAG, "Filesystem mounted");
    isInit = true;
    
    sdmmc_card_print_info(stdout, card);
    return true;
}

SDCard::~SDCard()
{
    if (!isInit)
    {
        return;
    }

    esp_vfs_fat_sdcard_unmount(mountPoint, card);
    spi_bus_free(spiDeviceSlot);
    ESP_LOGI(SD_TAG, "Card unmounted");
}