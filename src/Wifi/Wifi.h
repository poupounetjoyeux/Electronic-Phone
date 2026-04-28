#ifndef HEADER_WIFI
#define HEADER_WIFI

#include "esp_wifi.h"
#include <string>

#define WIFI_TAG "[Wifi]"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

struct WifiClientOptions
{
    const char* ssid;
    const char* pass;
};

class WifiClient
{
public:
    WifiClient(WifiClientOptions options);
    ~WifiClient();
    bool init();
private:
  bool isInit;
  wifi_config_t config;
};

#endif