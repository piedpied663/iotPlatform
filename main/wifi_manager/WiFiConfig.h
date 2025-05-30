#pragma once
#ifndef __WIFI_CONFIG_H__
#define __WIFI_CONFIG_H__

#include "types.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "esp_log.h"
#include "event_bus.h"

#ifdef CONFIG_IOT_FEATURE_SD
#define STA_CFG_PATH "/sd/sta.bin"
#define AP_CFG_PATH "/sd/ap.bin"
#else
#define STA_CFG_PATH "/fs/sta.bin"
#define AP_CFG_PATH "/fs/ap.bin"
#endif

class WiFiConfig
{
public:
    static constexpr const char *TAG = "[WiFiConfig]";

private:
    net_ap_config_t ap_cfg = {
        .ssid = CONFIG_IOT_AP_SSID,
        .password = CONFIG_IOT_AP_PSWD,
        .channel = CONFIG_IOT_AP_CHANNEL,
        .max_connection = CONFIG_IOT_AP_MAX_CONNECTION};
    net_credential_t sta_creds = {.ssid = CONFIG_IOT_WIFI_SSID, .password = CONFIG_IOT_WIFI_PASSWD};
    net_sta_config_t sta_cfg = {
        .credentials = {sta_creds},
        .currentNetwork = 0,
        .count = 1,
    };

    // Emission d'événements via EventBus
    static void on_event(const Event *evt);
    struct register_event_bus
    {
        register_event_bus()
        {
            EventBus::getInstance().subscribe(on_event, TAG);
        }
    };

    inline static register_event_bus register_event_bus_;
    WiFiConfig() {}

public:
    static WiFiConfig &getInstance()
    {
        static WiFiConfig instance;
        return instance;
    }
    WiFiConfig(const WiFiConfig &) = delete;
    WiFiConfig &operator=(const WiFiConfig &) = delete;

private:
    void task_load_ap();
    void task_load_sta();
    void task_save_sta();
    void task_save_ap();    
public:
    // void emitEvent(EventType type, void *user_ctx = nullptr);
    bool load_sta();
    bool load_ap();
    net_ap_config_t *get_ap() { return &ap_cfg; }
    net_sta_config_t *get_sta() { return &sta_cfg; }

    void update_sta(const net_sta_config_t *);
    void update_ap(const net_ap_config_t *);

    static bool sta_to_json(char *, size_t);
    static bool sta_from_json(const char *, net_sta_config_t *);
    static bool ap_to_json(char *, size_t);
    static bool ap_from_json(const char *, net_ap_config_t *);
};

#endif