#pragma once
#ifndef __TYPES_H__
#define __TYPES_H__

#include <cstddef>
#include <cstdint>

// Constantes
#define WIFI_MAX_NETWORKS 5
#define WIFI_MAX_SSID_LEN 32
#define WIFI_MAX_PASS_LEN 64

struct net_credential_t
{
    char ssid[WIFI_MAX_SSID_LEN];
    char password[WIFI_MAX_PASS_LEN];
};

struct net_sta_config_t
{
    net_credential_t credentials[WIFI_MAX_NETWORKS];
    size_t count;
};

struct net_ap_config_t
{
    char ssid[WIFI_MAX_SSID_LEN];
    char password[WIFI_MAX_PASS_LEN];
    uint8_t channel;
};

enum class WiFiStatus
{
    INIT,
    AP_STARTED,
    STA_CONNECTING,
    STA_CONNECTED,
    STA_DISCONNECTED,
    ALL_STA_FAILED,
    ERROR
};
// Types d’événements
enum class EventType : uint8_t
{
    NONE,           // for release bus
    BOOT_CAN_START, // first event, to start
    FS_READY,       // filesystem ready
    FS_ERROR,       // filesystem error do not append
    AP_SAVE,
    AP_SAVED,
    AP_LOAD,
    AP_LOADED,
    AP_ERROR,
    //         evt.type = EventType::AP_REQUEST;
    //         EventBus::getInstance().emit(evt);
    AP_REQUEST, // request AP config
    AP_ANSWER,  // answer AP config event
    AP_REQUEST_JSON,
    AP_ANSWER_JSON,
    AP_POST_REQUEST,
    AP_POST_ANSWER,
    STA_SAVE,
    STA_LOAD,
    STA_LOADED,
    STA_ERROR,
    //         evt.type = EventType::STA_REQUEST;
    //         EventBus::getInstance().emit(evt);
    // EventBus::getInstance().subscribe([&](const Event &evt)
    //                                   {

    //     if (evt.type == EventType::STA_ANSWER)
    //     {
    //         memcpy(&sta_cfg, evt.data, evt.data_len < sizeof(sta_cfg) ? evt.data_len : sizeof(sta_cfg));
    //         ESP_LOGI("TAG", "STA config: %s", sta_cfg.credentials[0].ssid);
    //         ESP_LOGI("TAG", "STA config: %s", sta_cfg.credentials[0].password);
    //     }else if (evt.type == EventType::AP_ANSWER)
    //     {
    //         memcpy(&ap_cfg, evt.data, sizeof(ap_cfg)); // bytes -> struct
    //         ESP_LOGI("TAG", "AP config: %s", ap_cfg.ssid);
    //         ESP_LOGI("TAG", "AP config: %s", ap_cfg.password);
    //     } });
    STA_REQUEST,
    STA_ANSWER,
    STA_REQUEST_JSON,
    STA_ANSWER_JSON,
    STA_POST_REQUEST,
    STA_POST_ANSWER,
    // CONFIG_READY,
    // CONFIG_ERROR,
    WIFI_STA_CONNECTED,
    WIFI_STA_DISCONNECTED,
    WIFI_STA_ERROR,
    WIFI_STATUS_REQUEST_JSON,
    WIFI_STATUS_ANSWER_JSON,
    MQTT_INIT_REQUEST,
    MQTT_INIT_DONE,
    // MQTT_CONFIG_LOADED,
    // MQTT_CONFIG_ERROR,
    MQTT_CONFIG_REQUEST_JSON,
    MQTT_CONFIG_ANSWER_JSON,
    MQTT_CONFIG_POST_REQUEST,
    MQTT_CONFIG_POST_ANSWER,
    MQTT_POST_REQUEST,
    MQTT_POST_ANSWER,
    MQTT_STATUS_REQUEST_JSON,
    MQTT_STATUS_ANSWER_JSON,
    MQTT_CONNECTED_REQUEST,
    MQTT_CONNECTED,
    MQTT_DISCONNECTED,
    MQTT_ERROR,
    // MQTT_PUBLISH_REQUEST,
    // MQTT_CONFIG_UPDATE,
    MQTT_MESSAGE,
    HTTPD_START,

    CAMERA_INIT_DONE,

    // etc.
};

// Event struct
struct Event
{
    EventType type;
    char source[16];
    uint8_t data[1024];
    size_t data_len;
    void* user_ctx;
};

// enum class MqttStatus
// {
//     CONNECTED,
//     CONNECTING,
//     DISCONNECTED
// };

#define MAX_BROKER_URI_LEN 128
#define MAX_USERNAME_LEN 32
#define MAX_PASSWORD_LEN 32
#define MAX_CLIENT_ID_LEN 32

struct mqtt_client_config_t
{
    char broker_uri[MAX_BROKER_URI_LEN];
    char username[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];
    char client_id[MAX_CLIENT_ID_LEN];
    bool enabled;
};

#endif