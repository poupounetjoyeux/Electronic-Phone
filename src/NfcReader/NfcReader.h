#ifndef HEADER_NFC_READER
#define HEADER_NFC_READER

#include <hal/gpio_types.h>
#include <string>
#include <string.h>
#include "driver/spi_master.h"

#define NFC_TAG "[NfcReader]"

#define PN532_HOST SPI3_HOST
#define PN532_PREAMBLE (0x00)   ///< Command sequence start, byte 1/3
#define PN532_STARTCODE1 (0x00) ///< Command sequence start, byte 2/3
#define PN532_STARTCODE2 (0xFF) ///< Command sequence start, byte 3/3
#define PN532_POSTAMBLE (0x00)  ///< EOD

#define PN532_HOSTTOPN532 (0xD4) ///< Host-to-PN532
#define PN532_PN532TOHOST (0xD5) ///< PN532-to-host

// PN532 Commands
#define PN532_COMMAND_GETFIRMWAREVERSION (0x02)  ///< Get firmware version
#define PN532_COMMAND_INLISTPASSIVETARGET (0x4A) ///< List passive target
#define PN532_COMMAND_SAMCONFIGURATION (0x14)    ///< SAM configuration

#define PN532_SPI_STATREAD (0x02)  ///< Stat read
#define PN532_SPI_DATAWRITE (0x01) ///< Data write
#define PN532_SPI_DATAREAD (0x03)  ///< Data read
#define PN532_SPI_READY (0x01)     ///< Ready

#define PN532_MIFARE_ISO14443A (0x00) ///< MiFare

struct NfcPins
{
  gpio_num_t sck;
  gpio_num_t mosi;
  gpio_num_t miso;
  gpio_num_t ss;
};

class NfcReader
{
public:
  NfcReader(NfcPins pinConfiguration);
  ~NfcReader();
  bool init();
  bool readPassiveTargetID(uint8_t cardbaudrate, uint8_t *uid, uint8_t *uidLength, uint16_t timeout = 100);

private:
  bool isInit;
  int phase;
  int readyCount;
  NfcPins pinConfiguration;
  spi_device_handle_t spiDevice;
  bool readack();
  void readdata(uint8_t *buff, uint8_t n);
  void writecommand(uint8_t *cmd, uint8_t cmdlen);
  bool waitready(uint16_t timeout);
  bool sendCommandCheckAck(uint8_t *cmd, uint8_t cmdlen, uint16_t timeout = 100);
  bool readDetectedPassiveTargetID(uint8_t *uid, uint8_t *uidLength);
  bool SAMConfig();
  uint32_t getFirmwareVersion();
  bool isready();
  int isReadyPhase(uint16_t timeout);
};

#endif