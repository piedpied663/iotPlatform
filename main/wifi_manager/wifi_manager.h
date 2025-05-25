#pragma once
#ifndef __WIFI_MANAGER_H__
#define __WIFI_MANAGER_H__

#include <cstddef>
#include <cstdint>
#include "event_bus.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "mdns.h"
#include "lwip/lwip_napt.h"
#include "lwip/dns.h"
#include <cstring>
#include "types.h" // Doit contenir net_credential_t, net_sta_config_t, net_ap_config_t, WiFiStatus, etc.

#define WIFI_MAX_NETWORKS 5
#define WIFI_MAX_SSID_LEN 32
#define WIFI_MAX_PASS_LEN 64

namespace wifi_manager
{
    static constexpr const char *TAG = "[WFMANG]";

    // -- Persistence/état --
    static bool _init = false;
    static bool _sta_persisted = false;
    static bool _ap_persisted = false;

    void init();
    bool load_sta();
    bool save_sta();
    bool load_ap();
    bool save_ap();

    const net_sta_config_t *get_sta();
    const net_ap_config_t *get_ap();

    void update_sta(const net_sta_config_t *cfg);
    void update_ap(const net_ap_config_t *cfg);

    bool sta_to_json(char *out_buf, size_t buf_len);
    bool sta_from_json(const char *json_str, net_sta_config_t *out_cfg);
    bool ap_to_json(char *out_buf, size_t buf_len);
    bool ap_from_json(const char *json_str, net_ap_config_t *out_cfg);

    void set_default_sta();
    void set_default_ap();

    void send_sta();
    void send_ap();

    // --- Logic WiFi ---
    static WiFiStatus g_status = WiFiStatus::INIT;
    static esp_netif_t *s_ap_netif = nullptr;
    static esp_netif_t *s_sta_netif = nullptr;
    static char s_ap_ip[16] = {0};
    static char s_sta_ip[16] = {0};

    void cleanup_netifs();

    void reconnect_async();
    void connect_next_sta();
    void start_apsta_async();
    void task_connect_wifi();

    // Helpers "vector-like"
    bool add_sta_network(const char *ssid, const char *password);
    bool remove_sta_network(const char *ssid);
    void clear_sta_networks();
    size_t get_sta_count();
    const net_credential_t *get_sta_list();

    // State pour retry/index
    size_t get_sta_current_index();

    // Status/IP/hostname
    WiFiStatus get_status();
    const char *get_ap_ip();
    const char *get_sta_ip();
    const char *get_sta_hostname();

    // Event bus
    void send_sta_event();
    extern void on_event(const Event &evt);
    static bool _subscribed = false;
    struct register_event_bus
    {
        register_event_bus()
        {
            if (!_subscribed)
            {
                EventBus::getInstance().subscribe(on_event, TAG);
                _subscribed = true;
            }
        }
    };
    static register_event_bus reg_event_bus;

    // Hashs pour filtrage event
    static uint32_t last_sta_hash;
    static uint32_t last_ap_hash;
}

#endif
