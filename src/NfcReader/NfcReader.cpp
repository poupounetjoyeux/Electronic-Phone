#include "NfcReader.h"
#include "esp_log.h"
#include <freertos/FreeRTOS.h>
#include "driver/gpio.h"

using namespace std;

uint8_t pn532ack[] = {0x00, 0x00, 0xFF,
                      0x00, 0xFF, 0x00}; ///< ACK message from PN532
uint8_t pn532response_firmwarevers[] = {
    0x00, 0x00, 0xFF,
    0x06, 0xFA, 0xD5}; ///< Expected firmware version message from PN532

#define PN532_PACKBUFFSIZ 64
uint8_t pn532_packetbuffer[PN532_PACKBUFFSIZ];

NfcReader::NfcReader(NfcPins pinConfiguration)
{
  this->pinConfiguration = pinConfiguration;
  phase = 0;
}

NfcReader::~NfcReader()
{
  if (!isInit)
  {
    return;
  }

  ESP_ERROR_CHECK(spi_bus_remove_device(spiDevice));
  ESP_ERROR_CHECK(spi_bus_free(PN532_HOST));
}

bool NfcReader::init()
{
  if (isInit)
  {
    ESP_LOGE(NFC_TAG, "NFC reader is already init");
    return false;
  }

  ESP_ERROR_CHECK(gpio_set_direction(pinConfiguration.ss, GPIO_MODE_OUTPUT));
  ESP_ERROR_CHECK(gpio_set_level(pinConfiguration.ss, 1));

  spi_bus_config_t buscfg = {
      .mosi_io_num = pinConfiguration.mosi,
      .miso_io_num = pinConfiguration.miso,
      .sclk_io_num = pinConfiguration.sck,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
  };
  ESP_ERROR_CHECK(spi_bus_initialize(PN532_HOST, &buscfg, SPI_DMA_CH_AUTO));

  spi_device_interface_config_t devcfg = {
      .command_bits = 8,
      .address_bits = 0,
      .dummy_bits = 0,
      .mode = 0,
      .clock_speed_hz = 2500000,
      .spics_io_num = pinConfiguration.ss,
      .flags = SPI_DEVICE_BIT_LSBFIRST,
      .queue_size = 1,
  };
  ESP_ERROR_CHECK(spi_bus_add_device(PN532_HOST, &devcfg, &spiDevice));

  ESP_ERROR_CHECK(gpio_set_level(pinConfiguration.ss, 0));
  vTaskDelay(2 / portTICK_PERIOD_MS);
  ESP_ERROR_CHECK(gpio_set_level(pinConfiguration.ss, 1));

  if (!SAMConfig())
  {
    ESP_LOGE(NFC_TAG, "Unable to configure SAM");
    return false;
  }

  uint32_t versiondata = getFirmwareVersion();
  if (!versiondata)
  {
    ESP_LOGE(NFC_TAG, "Didn't find NFC board");
    return false;
  }
  else
  {

    ESP_LOGI(NFC_TAG, "NFC board Version is %ld", versiondata);
  }
  isInit = true;
  return true;
}

bool NfcReader::SAMConfig()
{
  ESP_LOGI(NFC_TAG, "Configuring SAM");
  pn532_packetbuffer[0] = PN532_COMMAND_SAMCONFIGURATION;
  pn532_packetbuffer[1] = 0x01; // normal mode;
  pn532_packetbuffer[2] = 0x14; // timeout 50ms * 20 = 1 second
  pn532_packetbuffer[3] = 0x00; // not use IRQ pin!

  if (!sendCommandCheckAck(pn532_packetbuffer, 4))
  {
    return false;
  }

  // read data packet
  readdata(pn532_packetbuffer, 9);

  int offset = 6;
  return (pn532_packetbuffer[offset] == 0x15);
}

uint32_t NfcReader::getFirmwareVersion()
{
  uint32_t response;

  pn532_packetbuffer[0] = PN532_COMMAND_GETFIRMWAREVERSION;

  if (!sendCommandCheckAck(pn532_packetbuffer, 1))
  {
    return 0;
  }

  // read data packet
  readdata(pn532_packetbuffer, 13);

  // check some basic stuff
  if (0 != memcmp((char *)pn532_packetbuffer,
                  (char *)pn532response_firmwarevers, 6))
  {
    return 0;
  }

  int offset = 7;
  response = pn532_packetbuffer[offset++];
  response <<= 8;
  response |= pn532_packetbuffer[offset++];
  response <<= 8;
  response |= pn532_packetbuffer[offset++];
  response <<= 8;
  response |= pn532_packetbuffer[offset++];

  return response;
}

bool NfcReader::sendCommandCheckAck(uint8_t *cmd, uint8_t cmdlen,
                                    uint16_t timeout)
{
  writecommand(cmd, cmdlen);

  if (!waitready(timeout))
  {
    return false;
  }

  // read acknowledgement
  if (!readack())
  {
    return false;
  }

  // Wait for chip to say its ready!
  if (!waitready(timeout))
  {
    return false;
  }

  return true; // ack'd command
}

bool NfcReader::readack()
{
  uint8_t ackbuff[6];
  readdata(ackbuff, 6);
  return (0 == memcmp(ackbuff, pn532ack, 6));
}

int NfcReader::isReadyPhase(uint16_t timeout)
{
  if (isready())
  {
    return 1;
  }
  else
  {
    readyCount++;
    if (readyCount == timeout * 1000)
    {
      return -1;
    }
    return 0;
  }
}

bool NfcReader::readPassiveTargetID(uint8_t cardbaudrate, uint8_t *uid,
                                    uint8_t *uidLength, uint16_t timeout)
{
  if (!isInit)
  {
    ESP_LOGE(NFC_TAG, "NFC reader is not initialized");
    return false;
  }

  if (phase == 0)
  {
    pn532_packetbuffer[0] = PN532_COMMAND_INLISTPASSIVETARGET;
    pn532_packetbuffer[1] = 1; // max 1 cards at once (we can set this to 2 later)
    pn532_packetbuffer[2] = cardbaudrate;
    writecommand(pn532_packetbuffer, 3);
    phase++;
    readyCount = 0;
  }
  else if (phase == 1)
  {
    int readyRes = isReadyPhase(timeout);
    if (readyRes == 1)
    {
      phase++;
    }
    else if (readyRes == -1)
    {
      phase = 0;
    }
  }
  else if (phase == 2)
  {
    if (readack())
    {
      phase++;
      readyCount = 0;
    }
    else
    {
      phase = 0;
    }
  }
  else if (phase == 3)
  {
    int readyRes = isReadyPhase(timeout);
    if (readyRes == 1)
    {
      phase++;
    }
    else if (readyRes == -1)
    {
      phase = 0;
    }
  }
  else
  {
    phase = 0;
    return readDetectedPassiveTargetID(uid, uidLength);
  }
  return false;
}

bool NfcReader::readDetectedPassiveTargetID(uint8_t *uid, uint8_t *uidLength)
{
  // read data packet
  readdata(pn532_packetbuffer, 20);
  // check some basic stuff

  /* ISO14443A card response should be in the following format:

    byte            Description
    -------------   ------------------------------------------
    b0..6           Frame header and preamble
    b7              Tags Found
    b8              Tag Number (only one used in this example)
    b9..10          SENS_RES
    b11             SEL_RES
    b12             NFCID Length
    b13..NFCIDLen   NFCID                                      */

  if (pn532_packetbuffer[7] != 1)
  {
    return 0;
  }

  uint16_t sens_res = pn532_packetbuffer[9];
  sens_res <<= 8;
  sens_res |= pn532_packetbuffer[10];

  /* Card appears to be Mifare Classic */
  *uidLength = pn532_packetbuffer[12];
  for (uint8_t i = 0; i < pn532_packetbuffer[12]; i++)
  {
    uid[i] = pn532_packetbuffer[13 + i];
  }
  phase = 0;
  return 1;
}

/************** high level communication functions (handles both I2C and SPI) */

/**************************************************************************/
/*!
    @brief  Tries to read the SPI or I2C ACK signal
*/
/**************************************************************************/

bool NfcReader::isready()
{
  uint8_t reply;

  spi_transaction_t t;
  memset(&t, 0, sizeof(t)); // Zero out the transaction
  t.cmd = PN532_SPI_STATREAD;
  t.length = 8;
  t.rxlength = 8;
  t.tx_buffer = NULL;
  t.rx_buffer = &reply;
  ESP_ERROR_CHECK(spi_device_polling_transmit(spiDevice, &t));
  return reply == PN532_SPI_READY;
}

bool NfcReader::waitready(uint16_t timeout)
{
  uint16_t timer = 0;
  while (!isready())
  {
    if (timeout != 0)
    {
      timer += 1;
      if (timer > timeout)
      {
        return false;
      }
    }
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
  return true;
}

void NfcReader::readdata(uint8_t *buff, uint8_t n)
{
  spi_transaction_t t;
  memset(&t, 0, sizeof(t)); // Zero out the transaction
  t.cmd = PN532_SPI_DATAREAD;
  t.length = 8 * n;
  t.rxlength = 8 * n;
  t.tx_buffer = NULL;
  t.rx_buffer = buff;
  ESP_ERROR_CHECK(spi_device_polling_transmit(spiDevice, &t));
}

/**************************************************************************/
/*!
    @brief  Writes a command to the PN532, automatically inserting the
            preamble and required frame details (checksum, len, etc.)

    @param  cmd       Pointer to the command buffer
    @param  cmdlen    Command length in bytes
*/
/**************************************************************************/
void NfcReader::writecommand(uint8_t *cmd, uint8_t cmdlen)
{
  uint8_t checksum;
  uint8_t p[8 + cmdlen];

  p[0] = PN532_PREAMBLE;
  p[1] = PN532_STARTCODE1;
  p[2] = PN532_STARTCODE2;

  checksum = PN532_PREAMBLE + PN532_STARTCODE1 + PN532_STARTCODE2;

  p[3] = cmdlen + 1;
  p[4] = ~(cmdlen + 1) + 1;
  p[5] = PN532_HOSTTOPN532;
  checksum += PN532_HOSTTOPN532;

  for (uint8_t i = 0; i < cmdlen; i++)
  {
    p[i + 6] = cmd[i];
    checksum += cmd[i];
  }

  p[cmdlen + 6] = ~checksum;
  p[cmdlen + 7] = PN532_POSTAMBLE;

  spi_transaction_t t;
  memset(&t, 0, sizeof(t)); // Zero out the transaction

  t.cmd = PN532_SPI_DATAWRITE;
  t.length = 8 * (cmdlen + 8);
  t.rxlength = 0;
  t.tx_buffer = p;
  t.rx_buffer = NULL;
  ESP_ERROR_CHECK(spi_device_polling_transmit(spiDevice, &t));
}