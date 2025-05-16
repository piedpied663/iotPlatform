#include "wifi_manager.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "lwip/lwip_napt.h"
#include <vector>
#include <string>
#include <cstring>

namespace wifi_manager
{
    static const char *TAG = "WifiMgr";
    static Status g_status = Status::INIT;
    static StatusCallback g_callback = nullptr;

    static esp_netif_t *s_ap_netif = nullptr;
    static esp_netif_t *s_sta_netif = nullptr;

    static char s_ap_ip[16] = {0};
    static char s_sta_ip[16] = {0};

    static std::vector<StaNetwork> g_sta_networks;
    static size_t g_current_sta_index = 0;
    static bool g_all_sta_failed = false;
    static const int MAX_RETRY_PER_NETWORK = 3;
    static int g_current_retry = 0;

    void wifi_reconnect_async(void *)
    {
        while (true)
        {
            if (wifi_manager::get_status() == wifi_manager::Status::ALL_STA_FAILED || wifi_manager::get_status() == wifi_manager::Status::STA_DISCONNECTED)
            {
                ESP_LOGI("WifiMgr", "Re-trying STA networks (AP-only fallback detected)");
                wifi_manager::start_apsta_async(nullptr); // Ou une fonction dédiée
            }
            vTaskDelay(pdMS_TO_TICKS(15000)); // toutes les 15 sec
        }
    }
    bool add_sta_network(const std::string &ssid, const std::string &password)
    {
        auto &nets = get_sta_list_mutable();
        for (auto &n : nets)
        {
            if (n.ssid == ssid)
            {
                // Soit tu ignores, soit tu update le password
                if (n.password != password)
                {
                    n.password = password; // Met à jour le password si SSID existe
                    return true;
                }
                return false; // Signale qu'il existait déjà (facultatif)
            }
        }
        g_sta_networks.push_back({ssid, password});
        return true;
    }
    void remove_sta_network(const std::string &ssid)
    {
        for (size_t i = 0; i < g_sta_networks.size(); ++i)
        {
            if (g_sta_networks[i].ssid == ssid)
            {
                g_sta_networks.erase(g_sta_networks.begin() + i);
                return;
            }
        }
    }
    const std::vector<StaNetwork> &get_sta_list()
    {
        return g_sta_networks;
    }
    std::vector<StaNetwork> &get_sta_list_mutable()
    {
        return g_sta_networks;
    }
    void clear_sta_networks()
    {
        g_sta_networks.clear();
    }

    Status get_status() { return g_status; }
    const char *get_ap_ip() { return s_ap_ip; }
    const char *get_sta_ip() { return s_sta_ip; }
    // Essaie le réseau STA suivant
    static void connect_next_sta()
    {
        if (g_sta_networks.empty())
        {
            ESP_LOGW(TAG, "No STA networks configured, AP only.");
            g_status = Status::ALL_STA_FAILED;
            g_all_sta_failed = true;
            if (g_callback)
                g_callback(g_status);
            return;
        }
        if (g_current_sta_index >= g_sta_networks.size())
        {
            g_status = Status::ALL_STA_FAILED;
            g_all_sta_failed = true;
            ESP_LOGW(TAG, "All STA failed, AP only.");
            if (g_callback)
                g_callback(g_status);
            return;
        }

        const StaNetwork &net = g_sta_networks[g_current_sta_index];
        if (net.ssid.empty() || net.ssid.length() > MAX_SSID_LEN)
        {
            ESP_LOGE(TAG, "Invalid SSID: %s", net.ssid.c_str());
            return;
        }

        wifi_config_t sta_cfg = {};
        strncpy((char *)sta_cfg.sta.ssid, net.ssid.c_str(), sizeof(sta_cfg.sta.ssid));
        strncpy((char *)sta_cfg.sta.password, net.password.c_str(), sizeof(sta_cfg.sta.password));
        sta_cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

        ESP_LOGI(TAG, "Trying STA[%d/%d]: %s", (int)g_current_sta_index + 1, (int)g_sta_networks.size(), net.ssid.c_str());
        esp_wifi_set_config(WIFI_IF_STA, &sta_cfg);
        esp_wifi_connect();

        g_status = Status::STA_CONNECTING;
        if (g_callback)
            g_callback(g_status);
    }

    static void on_wifi_event(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
    {
        if (event_base == WIFI_EVENT)
        {
            switch (event_id)
            {
            case WIFI_EVENT_AP_START:
                g_status = Status::AP_STARTED;
                ESP_LOGI(TAG, "AP started");
                if (g_callback)
                    g_callback(g_status);
                break;
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "STA starting...");
                g_current_retry = 0;
                connect_next_sta();
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                if (g_current_retry < MAX_RETRY_PER_NETWORK)
                {
                    g_current_retry++;
                    ESP_LOGW(TAG, "STA disconnected (retry %d/%d)", g_current_retry, MAX_RETRY_PER_NETWORK);
                    esp_wifi_connect(); // Retente la même
                }
                else
                {
                    g_current_retry = 0;
                    g_current_sta_index++;
                    connect_next_sta();
                }
                break;
            }
        }
        else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
        {
            g_status = Status::STA_CONNECTED;
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
            snprintf(s_sta_ip, sizeof(s_sta_ip), IPSTR, IP2STR(&event->ip_info.ip));
            ESP_LOGI(TAG, "STA got IP: %s", s_sta_ip);

            // Active NAT sur AP seulement si STA a une IP
            esp_netif_ip_info_t ap_ip;
            esp_netif_get_ip_info(s_ap_netif, &ap_ip);
            ip_napt_enable(ap_ip.ip.addr, 1); // NAT ON

            if (g_callback)
                g_callback(g_status);
        }
    }
    void cleanup_netifs()
    {
        if (s_sta_netif)
        {
            esp_netif_destroy(s_sta_netif);
            s_sta_netif = nullptr;
        }
    }
    void start_apsta_async(StatusCallback cb)
    {
        cleanup_netifs();
        g_callback = cb;
        g_current_sta_index = 0;
        g_all_sta_failed = false;

        esp_netif_init();
        esp_event_loop_create_default();

        s_ap_netif = esp_netif_create_default_wifi_ap();
        s_sta_netif = esp_netif_create_default_wifi_sta();

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        esp_wifi_init(&cfg);

        // Register wifi/IP events
        esp_event_handler_instance_t instance_any_id;
        esp_event_handler_instance_t instance_got_ip;
        esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &on_wifi_event, NULL, &instance_any_id);
        esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &on_wifi_event, NULL, &instance_got_ip);

        // -- AP config --
        wifi_config_t ap_config = {};
        strncpy((char *)ap_config.ap.ssid, "IotCamera", sizeof(ap_config.ap.ssid));
        strncpy((char *)ap_config.ap.password, "IotCamera", sizeof(ap_config.ap.password));
        ap_config.ap.ssid_len = strlen("IotCamera");
        ap_config.ap.max_connection = 4;
        ap_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
        ap_config.ap.channel = 1;

        esp_wifi_set_mode(WIFI_MODE_APSTA);
        esp_wifi_set_config(WIFI_IF_AP, &ap_config);

        esp_wifi_start();

        // Set AP IP (pour NAT, info)
        esp_netif_ip_info_t ip_info;
        esp_netif_get_ip_info(s_ap_netif, &ip_info);
        snprintf(s_ap_ip, sizeof(s_ap_ip), IPSTR, IP2STR(&ip_info.ip));
    }
}
