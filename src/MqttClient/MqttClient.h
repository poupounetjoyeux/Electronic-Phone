#ifndef HEADER_MQTT_CLIENT
#define HEADER_MQTT_CLIENT

#include "mqtt_client.h"
#include <string>

#define MQTT_TAG "[MqttClient]"

struct MqttClientOptions
{
    const char* broker;
    const char* clientId;
};

class MqttClient
{
public:
    MqttClient(MqttClientOptions options);
    ~MqttClient();
    bool init();
    void publish(std::string topic, std::string payload, bool retain);
private:
  bool isInit;
  esp_mqtt_client_handle_t client;
  esp_mqtt_client_config_t config;
};

#endif