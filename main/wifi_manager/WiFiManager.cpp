#include "WiFiManager.h"
#include "types.h"
#include "utils.h"
#include "event_bus.h"
#include "esp_log.h"
const net_ap_config_t *ap_cfg = nullptr;
const net_sta_config_t *sta_cfg = nullptr;
WiFiStatus g_status = WiFiStatus::INIT;
esp_netif_t *s_ap_netif = nullptr;
esp_netif_t *s_sta_netif = nullptr;
size_t g_current_sta_index = 0;
bool g_all_sta_failed = false;
int g_current_retry = 0;
const int MAX_RETRY_PER_NETWORK = 3;
bool g_ap_ready = false;
bool g_sta_ready = false;
bool g_sta_ap_ready = false;

WiFiManager &WiFiManager::getInstance()
{
    static WiFiManager instance;
    return instance;
}

void WiFiManager::on_event(const Event *evt)
{

    if (evt->type == EventType::AP_LOAD_SUCCESS || evt->type == EventType::AP_LOAD_FAIL)
    {
        if (evt->user_ctx != nullptr)
        {
            ap_cfg = static_cast<const net_ap_config_t *>(evt->user_ctx);
            ESP_LOGI(TAG, "AP received: %s", ap_cfg->ssid);
            if (!g_ap_ready)
            {
                g_ap_ready = true;
            }
        }
    }
    else if (evt->type == EventType::STA_LOAD_SUCCESS || evt->type == EventType::STA_LOAD_FAIL)
    {
        if (evt->user_ctx != nullptr)
        {
            sta_cfg = static_cast<const net_sta_config_t *>(evt->user_ctx);
            ESP_LOGI(TAG, "STA received: %s", sta_cfg->credentials[0].ssid);
            if (!g_sta_ready)
            {
                g_sta_ready = true;
            }
        }
    }

    if (g_sta_ready && g_ap_ready && !g_sta_ap_ready)
    {
        if (!g_sta_ap_ready)
        {
            WiFiManager::getInstance().task_connect_wifi();
            g_sta_ap_ready = true;
            utils::emitEvent(EventType::HTTPD_START, nullptr);
        }
    }
}

void WiFiManager::cleanup_netifs()
{
    if (s_sta_netif)
    {
        esp_netif_destroy(s_sta_netif);
        s_sta_netif = nullptr;
    }
    if (s_ap_netif)
    {
        esp_netif_destroy(s_ap_netif);
        s_ap_netif = nullptr;
    }
}

void WiFiManager::connect_next_sta()
{
    if (sta_cfg->count == 0)
    {
        ESP_LOGW(TAG, "No STA networks configured, AP only.");
        g_status = WiFiStatus::ALL_STA_FAILED;
        g_all_sta_failed = true;
        return;
    }
    if (g_current_sta_index >= sta_cfg->count)
    {
        g_status = WiFiStatus::ALL_STA_FAILED;
        g_all_sta_failed = true;
        ESP_LOGW(TAG, "All STA failed, AP only.");
        return;
    }
    const net_credential_t &net = sta_cfg->credentials[g_current_sta_index];
    if (strlen(net.ssid) == 0)
    {
        ESP_LOGE(TAG, "Invalid SSID (empty)");
        return;
    }
    wifi_config_t sta_wifi_cfg = {};
    strncpy((char *)sta_wifi_cfg.sta.ssid, net.ssid, sizeof(sta_wifi_cfg.sta.ssid));
    strncpy((char *)sta_wifi_cfg.sta.password, net.password, sizeof(sta_wifi_cfg.sta.password));
    sta_wifi_cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    ESP_LOGI(TAG, "Trying STA[%d/%d]: %s", (int)g_current_sta_index + 1, (int)sta_cfg->count, net.ssid);
    esp_wifi_set_config(WIFI_IF_STA, &sta_wifi_cfg);
    esp_wifi_connect();
    g_status = WiFiStatus::STA_CONNECTING;
}

void WiFiManager::on_wifi_event(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
        case WIFI_EVENT_AP_START:
            g_status = WiFiStatus::AP_STARTED;
            ESP_LOGI(TAG, "AP started");
            break;
        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG, "STA starting...");
            g_current_retry = 0;
            WiFiManager::getInstance().connect_next_sta();
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
                WiFiManager::getInstance().connect_next_sta();
            }

            break;
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        g_status = WiFiStatus::STA_CONNECTED;
        char s_sta_ip[16];
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        snprintf(s_sta_ip, sizeof(s_sta_ip), IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "STA got IP: %s", s_sta_ip);
        iot_mqtt_status_t status{};
        strncpy(status.ip, s_sta_ip, sizeof(status.ip) - 1);
        strncpy(status.host, ap_cfg->ssid, sizeof(status.host) - 1);
        status.ip[sizeof(status.ip) - 1] = '\0';
        status.host[sizeof(status.host) - 1] = '\0';

        utils::emitEventData(EventType::WIFI_STA_CONNECTED, &status, sizeof(iot_mqtt_status_t));
#if ESP_LWIP && LWIP_IPV4 && IP_NAPT
        // Active NAT sur AP seulement si STA a une IP
        esp_netif_ip_info_t ap_ip;
        esp_netif_get_ip_info(s_ap_netif, &ap_ip);

        ip_napt_enable(ap_ip.ip.addr, 1); // NAT ON

#else
#warning "IP_NAPT not enabled"
#endif
    }
}

void WiFiManager::task_connect_wifi()
{
    xTaskCreate([](void *)
                {
            ESP_LOGI(TAG, "Connecting to WiFi...");
            WiFiManager::getInstance().start_apsta_async();
            vTaskDelete(nullptr); }, "wifi_connect_async", 4096, nullptr, 5, nullptr);
}

void WiFiManager::start_apsta_async()
{
    cleanup_netifs();
    g_current_sta_index = 0;
    g_all_sta_failed = false;

    esp_netif_init();
    esp_event_loop_create_default();

    s_ap_netif = esp_netif_create_default_wifi_ap();
    s_sta_netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &on_wifi_event, NULL, &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &on_wifi_event, NULL, &instance_got_ip);

    // AP config
    wifi_config_t ap_config = {};
    strncpy((char *)ap_config.ap.ssid, ap_cfg->ssid, sizeof(ap_config.ap.ssid));
    strncpy((char *)ap_config.ap.password, ap_cfg->password, sizeof(ap_config.ap.password));
    ap_config.ap.ssid_len = strlen(ap_cfg->ssid);
    ap_config.ap.max_connection = ap_cfg->max_connection;
    ap_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    ap_config.ap.channel = ap_cfg->channel;

    esp_wifi_set_mode(WIFI_MODE_APSTA);
    esp_wifi_set_config(WIFI_IF_AP, &ap_config);

    esp_wifi_start();

    // // Set AP IP
    // esp_netif_ip_info_t ip_info;
    // esp_netif_get_ip_info(s_ap_netif, &ip_info);
    // snprintf(s_ap_ip, sizeof(s_ap_ip), IPSTR, IP2STR(&ip_info.ip));
}