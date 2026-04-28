#include "MqttClient.h"
#include "esp_log.h"

using namespace std;

void MqttClient::publish(string topic, string payload, bool retain)
{
    esp_mqtt_client_publish(client, topic.c_str(), payload.c_str(), payload.length(), 0, retain ? 1 : 0);
}

MqttClient::MqttClient(MqttClientOptions options)
{
    config = {
        .broker = {
            .address = {
                .uri = options.broker}},
        .credentials = {.client_id = options.clientId},
        .network = {
            .reconnect_timeout_ms = 3000,
            .disable_auto_reconnect = false
        }
    };
}

bool MqttClient::init()
{
    if (isInit)
    {
        ESP_LOGE(MQTT_TAG, "MQTT client is already init");
        return false;
    }

    client = esp_mqtt_client_init(&config);
    ESP_ERROR_CHECK(esp_mqtt_client_start(client));
    isInit = true;
    return true;
}

MqttClient::~MqttClient()
{
    ESP_ERROR_CHECK(esp_mqtt_client_disconnect(client));
    ESP_ERROR_CHECK(esp_mqtt_client_destroy(client));
}