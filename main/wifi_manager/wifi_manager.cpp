#include "wifi_manager.h"
#include <stdio.h>
#include <string.h>
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "utils.h"
#include "persistence.h"
#include "macro.h"

#ifdef CONFIG_IOT_FEATURE_SD
#define STA_CFG_PATH "/sd/sta.bin"
#define AP_CFG_PATH "/sd/ap.bin"
#else
#define STA_CFG_PATH "/fs/sta.bin"
#define AP_CFG_PATH "/fs/ap.bin"
#endif

namespace wifi_manager
{
    // RAM principale
    static net_sta_config_t sta_cfg = {};
    static net_ap_config_t ap_cfg = {};

    // Index/retry
    static size_t g_current_sta_index = 0;
    static bool g_all_sta_failed = false;
    static int g_current_retry = 0;
    static const int MAX_RETRY_PER_NETWORK = 3;

    // --- Persistence/init ---
    void init(void)
    {
        xTaskCreate([](void *)
                    {
            Event evt = {};
            bool res = load_ap();
            evt.type = res ? EventType::AP_LOADED : EventType::AP_ERROR;
            ESP_LOGI(TAG, "load_ap: %d", (uint8_t)evt.type);
            EventBus::getInstance().emit(evt);
            vTaskDelete(nullptr); }, "load_ap", 4096, nullptr, tskIDLE_PRIORITY + 1, nullptr);

        xTaskCreate([](void *)
                    {
            Event evt = {};
            bool res = load_sta();
            evt.type = res ? EventType::STA_LOADED : EventType::STA_ERROR;
            ESP_LOGI(TAG, "load_sta: %d", (uint8_t)evt.type);
            EventBus::getInstance().emit(evt);
            vTaskDelete(nullptr); }, "load_sta", 4096, nullptr, tskIDLE_PRIORITY + 1, nullptr);

        _init = true;
    }

    bool load_sta(void)
    {
        return persist::load_struct(STA_CFG_PATH, &sta_cfg, sizeof(sta_cfg));
    }
    void task_save_sta(void)
    {
        xTaskCreate([](void *) { save_sta(); vTaskDelete(nullptr); }, "save_sta", 4096, nullptr, tskIDLE_PRIORITY + 1, nullptr);
    }
    bool save_sta(void)
    {
        return persist::save_struct(STA_CFG_PATH, &sta_cfg, sizeof(sta_cfg));
    }
    bool load_ap(void)
    {
        return persist::load_struct(AP_CFG_PATH, &ap_cfg, sizeof(ap_cfg));
    }
    void task_save_ap(void)
    {
        xTaskCreate([](void *) { save_ap(); vTaskDelete(nullptr); }, "save_ap", 4096, nullptr, tskIDLE_PRIORITY + 1, nullptr);
    }
    bool save_ap(void)
    {
        return persist::save_struct(AP_CFG_PATH, &ap_cfg, sizeof(ap_cfg));
    }

    // --- Get/set RAM ---
    const net_sta_config_t *get_sta(void) { return &sta_cfg; }
    const net_ap_config_t *get_ap(void) { return &ap_cfg; }

    void update_sta(const net_sta_config_t *cfg)
    {
        memcpy(&sta_cfg, cfg, sizeof(*cfg));
        save_sta();
    }
    void update_ap(const net_ap_config_t *cfg)
    {
        memcpy(&ap_cfg, cfg, sizeof(*cfg));
        save_ap();
    }

    // --- JSON helpers ---
    bool sta_to_json(char *out_buf, size_t buf_len)
    {
        cJSON *root = cJSON_CreateObject();
        cJSON *arr = cJSON_AddArrayToObject(root, "stations");
        for (size_t i = 0; i < sta_cfg.count; ++i)
        {
            cJSON *obj = cJSON_CreateObject();
            cJSON_AddStringToObject(obj, "ssid", sta_cfg.credentials[i].ssid);
            cJSON_AddStringToObject(obj, "password", sta_cfg.credentials[i].password);
            cJSON_AddItemToArray(arr, obj);
        }
        cJSON_AddNumberToObject(root, "count", sta_cfg.count);
        cJSON_AddNumberToObject(root, "currentNetwork", g_current_sta_index);
        char *json = cJSON_PrintUnformatted(root);
        if (!json)
        {
            cJSON_Delete(root);
            return false;
        }
        strncpy(out_buf, json, buf_len - 1);
        out_buf[buf_len - 1] = 0;
        cJSON_free(json);
        cJSON_Delete(root);
        return true;
    }

    bool sta_from_json(const char *json_str, net_sta_config_t *out_cfg)
    {
        cJSON *root = cJSON_Parse(json_str);
        if (!root)
            return false;
        cJSON *arr = cJSON_GetObjectItemCaseSensitive(root, "stations");
        if (!cJSON_IsArray(arr))
        {
            cJSON_Delete(root);
            return false;
        }
        out_cfg->count = 0;
        cJSON *item;
        cJSON_ArrayForEach(item, arr)
        {
            if (out_cfg->count >= WIFI_MAX_NETWORKS)
                break;
            cJSON *j_ssid = cJSON_GetObjectItemCaseSensitive(item, "ssid");
            cJSON *j_pwd = cJSON_GetObjectItemCaseSensitive(item, "password");
            if (cJSON_IsString(j_ssid) && cJSON_IsString(j_pwd))
            {
                strncpy(out_cfg->credentials[out_cfg->count].ssid, j_ssid->valuestring, WIFI_MAX_SSID_LEN - 1);
                strncpy(out_cfg->credentials[out_cfg->count].password, j_pwd->valuestring, WIFI_MAX_PASS_LEN - 1);
                out_cfg->credentials[out_cfg->count].ssid[WIFI_MAX_SSID_LEN - 1] = 0;
                out_cfg->credentials[out_cfg->count].password[WIFI_MAX_PASS_LEN - 1] = 0;
                out_cfg->count++;
            }
        }
        cJSON_Delete(root);
        return true;
    }

    bool ap_to_json(char *out_buf, size_t buf_len)
    {
        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "ssid", ap_cfg.ssid);
        cJSON_AddStringToObject(root, "password", ap_cfg.password);
        cJSON_AddNumberToObject(root, "channel", ap_cfg.channel);
        char *json = cJSON_PrintUnformatted(root);
        if (!json)
        {
            cJSON_Delete(root);
            return false;
        }
        strncpy(out_buf, json, buf_len - 1);
        out_buf[buf_len - 1] = 0;
        cJSON_free(json);
        cJSON_Delete(root);
        return true;
    }
    bool ap_from_json(const char *json_str, net_ap_config_t *out_cfg)
    {
        cJSON *root = cJSON_Parse(json_str);
        if (!root)
            return false;
        cJSON *j_ssid = cJSON_GetObjectItemCaseSensitive(root, "ssid");
        cJSON *j_pwd = cJSON_GetObjectItemCaseSensitive(root, "password");
        cJSON *j_chn = cJSON_GetObjectItemCaseSensitive(root, "channel");
        if (cJSON_IsString(j_ssid) && cJSON_IsString(j_pwd) && cJSON_IsNumber(j_chn))
        {
            strncpy(out_cfg->ssid, j_ssid->valuestring, WIFI_MAX_SSID_LEN - 1);
            strncpy(out_cfg->password, j_pwd->valuestring, WIFI_MAX_PASS_LEN - 1);
            out_cfg->ssid[WIFI_MAX_SSID_LEN - 1] = 0;
            out_cfg->password[WIFI_MAX_PASS_LEN - 1] = 0;
            out_cfg->channel = (uint8_t)j_chn->valuedouble;
            cJSON_Delete(root);
            return true;
        }
        cJSON_Delete(root);
        return false;
    }

    // --- Default/fallback ---
    void set_default_sta(void)
    {
        memset(&sta_cfg, 0, sizeof(sta_cfg));
        sta_cfg.count = 1;
        strncpy(sta_cfg.credentials[0].ssid, CONFIG_IOT_WIFI_SSID, WIFI_MAX_SSID_LEN - 1);
        strncpy(sta_cfg.credentials[0].password, CONFIG_IOT_WIFI_PASSWD, WIFI_MAX_PASS_LEN - 1);
        sta_cfg.credentials[0].ssid[WIFI_MAX_SSID_LEN - 1] = 0;
        sta_cfg.credentials[0].password[WIFI_MAX_PASS_LEN - 1] = 0;
        _sta_persisted = true;
        save_sta();
    }
    void set_default_ap(void)
    {
        memset(&ap_cfg, 0, sizeof(ap_cfg));
        strncpy(ap_cfg.ssid, CONFIG_IOT_AP_SSID, WIFI_MAX_SSID_LEN - 1);
        strncpy(ap_cfg.password, CONFIG_IOT_AP_PSWD, WIFI_MAX_PASS_LEN - 1);
        ap_cfg.channel = CONFIG_IOT_AP_CHANNEL;
        ap_cfg.ssid[WIFI_MAX_SSID_LEN - 1] = 0;
        ap_cfg.password[WIFI_MAX_PASS_LEN - 1] = 0;
        _ap_persisted = true;
        save_ap();
    }

    // --- Helpers "vector-like" ---
    bool add_sta_network(const char *ssid, const char *password)
    {
        for (size_t i = 0; i < sta_cfg.count; ++i)
        {
            if (strncmp(sta_cfg.credentials[i].ssid, ssid, WIFI_MAX_SSID_LEN) == 0)
            {
                if (strncmp(sta_cfg.credentials[i].password, password, WIFI_MAX_PASS_LEN) != 0)
                {
                    strncpy(sta_cfg.credentials[i].password, password, WIFI_MAX_PASS_LEN - 1);
                    sta_cfg.credentials[i].password[WIFI_MAX_PASS_LEN - 1] = 0;
                    save_sta();
                    return true;
                }
                return false;
            }
        }
        if (sta_cfg.count < WIFI_MAX_NETWORKS)
        {
            strncpy(sta_cfg.credentials[sta_cfg.count].ssid, ssid, WIFI_MAX_SSID_LEN - 1);
            sta_cfg.credentials[sta_cfg.count].ssid[WIFI_MAX_SSID_LEN - 1] = 0;
            strncpy(sta_cfg.credentials[sta_cfg.count].password, password, WIFI_MAX_PASS_LEN - 1);
            sta_cfg.credentials[sta_cfg.count].password[WIFI_MAX_PASS_LEN - 1] = 0;
            sta_cfg.count++;
            save_sta();
            return true;
        }
        return false;
    }
    bool remove_sta_network(const char *ssid)
    {
        for (size_t i = 0; i < sta_cfg.count; ++i)
        {
            if (strncmp(sta_cfg.credentials[i].ssid, ssid, WIFI_MAX_SSID_LEN) == 0)
            {
                for (size_t j = i + 1; j < sta_cfg.count; ++j)
                {
                    sta_cfg.credentials[j - 1] = sta_cfg.credentials[j];
                }
                sta_cfg.count--;
                save_sta();
                return true;
            }
        }
        return false;
    }
    void clear_sta_networks()
    {
        memset(sta_cfg.credentials, 0, sizeof(sta_cfg.credentials));
        sta_cfg.count = 0;
        save_sta();
    }
    size_t get_sta_count() { return sta_cfg.count; }
    const net_credential_t *get_sta_list() { return sta_cfg.credentials; }
    size_t get_sta_current_index() { return g_current_sta_index; }

    // --- Logic WiFi ---
    WiFiStatus get_status() { return g_status; }
    const char *get_ap_ip() { return s_ap_ip; }
    const char *get_sta_ip() { return s_sta_ip; }
    const char *get_sta_hostname()
    {
        esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
        if (netif == nullptr)
        {
            ESP_LOGE("HOSTNAME", "Failed to get WIFI_STA_DEF interface");
            return CONFIG_IOT_HOSTNAME;
        }
        const char *hostname = nullptr;
        esp_err_t err = esp_netif_get_hostname(netif, &hostname);
        if (err != ESP_OK || hostname == nullptr)
        {
            ESP_LOGE("HOSTNAME", "Failed to get hostname (err=%d)", err);
            return CONFIG_IOT_HOSTNAME;
        }
        return hostname;
    }

    void cleanup_netifs()
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

    void connect_next_sta()
    {
        if (sta_cfg.count == 0)
        {
            ESP_LOGW(TAG, "No STA networks configured, AP only.");
            g_status = WiFiStatus::ALL_STA_FAILED;
            g_all_sta_failed = true;
            return;
        }
        if (g_current_sta_index >= sta_cfg.count)
        {
            g_status = WiFiStatus::ALL_STA_FAILED;
            g_all_sta_failed = true;
            ESP_LOGW(TAG, "All STA failed, AP only.");
            return;
        }
        const net_credential_t &net = sta_cfg.credentials[g_current_sta_index];
        if (strlen(net.ssid) == 0)
        {
            ESP_LOGE(TAG, "Invalid SSID (empty)");
            return;
        }
        wifi_config_t sta_wifi_cfg = {};
        strncpy((char *)sta_wifi_cfg.sta.ssid, net.ssid, sizeof(sta_wifi_cfg.sta.ssid));
        strncpy((char *)sta_wifi_cfg.sta.password, net.password, sizeof(sta_wifi_cfg.sta.password));
        sta_wifi_cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
        ESP_LOGI(TAG, "Trying STA[%d/%d]: %s", (int)g_current_sta_index + 1, (int)sta_cfg.count, net.ssid);
        esp_wifi_set_config(WIFI_IF_STA, &sta_wifi_cfg);
        esp_wifi_connect();
        g_status = WiFiStatus::STA_CONNECTING;
    }

    void reconnect_async()
    {
        xTaskCreate([](void *)
                    {
            while (true)
            {
                if (get_status() == WiFiStatus::ALL_STA_FAILED || get_status() == WiFiStatus::STA_DISCONNECTED)
                {
                    ESP_LOGI(TAG, "Re-trying STA networks (AP-only fallback detected)");
                    start_apsta_async();
                }
                vTaskDelay(pdMS_TO_TICKS(15000));
            } }, "wifi_reconnect_async", 4096, nullptr, 5, nullptr);
    }

    __attribute__((noinline)) static void on_wifi_event(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
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
            g_status = WiFiStatus::STA_CONNECTED;
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
            snprintf(s_sta_ip, sizeof(s_sta_ip), IPSTR, IP2STR(&event->ip_info.ip));
            ESP_LOGI(TAG, "STA got IP: %s", s_sta_ip);
            send_sta_event();
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
#include "cJSON.h"

    void send_sta_event()
    {
        Event evt = {};
        evt.type = EventType::WIFI_STA_CONNECTED;

        // Compose JSON { "ip": "...", "host": "..." }
        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "ip", s_sta_ip);
        // Récupère ton hostname ici (ap_cfg.ssid ou via get_sta_hostname())
        cJSON_AddStringToObject(root, "host", ap_cfg.ssid); // ou get_sta_hostname()

        char *json = cJSON_PrintUnformatted(root);
        if (json)
        {
            size_t len = strnlen(json, sizeof(evt.data) - 1);
            memcpy(evt.data, json, len);
            evt.data[len] = 0;
            evt.data_len = len + 1;
            cJSON_free(json);
        }
        else
        {
            evt.data[0] = 0;
            evt.data_len = 1;
        }
        cJSON_Delete(root);

        EventBus::getInstance().emit(evt);
    }

    void start_apsta_async()
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
        strncpy((char *)ap_config.ap.ssid, ap_cfg.ssid, sizeof(ap_config.ap.ssid));
        strncpy((char *)ap_config.ap.password, ap_cfg.password, sizeof(ap_config.ap.password));
        ap_config.ap.ssid_len = strlen(ap_cfg.ssid);
        ap_config.ap.max_connection = 4;
        ap_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
        ap_config.ap.channel = ap_cfg.channel;

        esp_wifi_set_mode(WIFI_MODE_APSTA);
        esp_wifi_set_config(WIFI_IF_AP, &ap_config);

        esp_wifi_start();

        // Set AP IP
        esp_netif_ip_info_t ip_info;
        esp_netif_get_ip_info(s_ap_netif, &ip_info);
        snprintf(s_ap_ip, sizeof(s_ap_ip), IPSTR, IP2STR(&ip_info.ip));
    }

    void task_connect_wifi()
    {
        xTaskCreate([](void *)
                    {
            ESP_LOGI(TAG, "Connecting to WiFi...");
            start_apsta_async();
            vTaskDelete(nullptr); }, "wifi_connect_async", 4096, nullptr, 5, nullptr);
    }
    static bool event_httpd_send = false;
    // --- Event Bus/Handlers ---
    void on_event(const Event &evt)
    {
        switch (evt.type)
        {
        case EventType::FS_READY:
        {
            if (!_init)
            {
                init();
                ESP_LOGI(TAG, "Initialized");
            }
            break;
        }
        case EventType::STA_LOADED:
        {
            if (_sta_persisted)
                return;

            if (!_sta_persisted)
            {
                _sta_persisted = true;
                ESP_LOGI(TAG, "STA persistence loaded");
            }

            break;
        }
        case EventType::AP_LOADED:
        {
            if (_ap_persisted)
                return;

            if (!_ap_persisted)
            {
                ESP_LOGI(TAG, "AP persistence loaded");
                _ap_persisted = true;
            }
            break;
        }
        case EventType::STA_ERROR:
        {
            if (!_sta_persisted)
            {
                ESP_LOGW(TAG, "STA persistence fallback");
                set_default_sta();
                _sta_persisted = true;
            }
            break;
        }
        case EventType::AP_ERROR:
        {
            if (!_ap_persisted)
            {
                ESP_LOGW(TAG, "AP persistence fallback");
                set_default_ap();
                _ap_persisted = true;
            }
            break;
        }
        case EventType::STA_REQUEST:
            send_sta();
            break;
        case EventType::AP_REQUEST:
            send_ap();
            break;

        case EventType::STA_REQUEST_JSON:
        {
            if (evt.user_ctx)
            {
                Event resp_evt = {};
                resp_evt.type = EventType::STA_ANSWER_JSON;
                char json[512];
                sta_to_json(json, sizeof(json));                
                BIND_DATA_EVENT_JSON(resp_evt, json);
                // Envoie la réponse
                xQueueSend((QueueHandle_t)evt.user_ctx, &resp_evt, 0);
            }
            break;
        }
        case EventType::STA_POST_REQUEST:
        {
            if (evt.user_ctx)
            {
                // 1) parser le JSON (evt.data, evt.data_len)
                const char *json = reinterpret_cast<const char *>(evt.data);
                // ex : sta_from_json(json);

                // 2) appliquer la config Wi-Fi, écrire en flash…
                
                if(sta_from_json(json, &sta_cfg))
                {
                    task_save_sta();
                }
                // 3) envoyer l’ack
                Event resp_evt{};
                BIND_DATA_EVENT_JSON(resp_evt, json);
                resp_evt.type = EventType::STA_POST_ANSWER;
                xQueueSend((QueueHandle_t)evt.user_ctx, &resp_evt, 0);
            }
            break;
        }
        case EventType::AP_REQUEST_JSON:
        {
            if (evt.user_ctx)
            {
                Event resp_evt = {};
                resp_evt.type = EventType::AP_ANSWER_JSON;
                char json[512];
                ap_to_json(json, sizeof(json));
                size_t len = strnlen(json, sizeof(json) - 1);
                memcpy(resp_evt.data, json, len);
                resp_evt.data_len = len;
                // Envoie la réponse
                xQueueSend((QueueHandle_t)evt.user_ctx, &resp_evt, 0);
            }
            break;
        }
        case EventType::AP_POST_REQUEST:
        {
            if (evt.user_ctx)
            {
                // 1) parser le JSON (evt.data, evt.data_len)
                const char *json = reinterpret_cast<const char *>(evt.data);
                // ex : sta_from_json(json);

                // 2) appliquer la config Wi-Fi, écrire en flash…
                
                if(ap_from_json(json, &ap_cfg))
                {
                    task_save_ap();
                }
                // 3) envoyer l’ack
                Event resp_evt{};
                BIND_DATA_EVENT_JSON(resp_evt, json);
                resp_evt.type = EventType::AP_POST_ANSWER;
                xQueueSend((QueueHandle_t)evt.user_ctx, &resp_evt, 0);
            }
            break;
        }
        default:
            break;
        }
        if (_sta_persisted && _ap_persisted)
        {
            task_connect_wifi();
            _sta_persisted = false;
            _ap_persisted = false;
        }

        if ((g_status == WiFiStatus::AP_STARTED || g_status == WiFiStatus::STA_CONNECTED) && !event_httpd_send)
        {
            Event e = {};
            e.type = EventType::HTTPD_START;
            EventBus::getInstance().emit(e);
            event_httpd_send = true;
        }
    }

    // register_event_bus reg_event_bus;
    // uint32_t last_sta_hash = 0;
    // uint32_t last_ap_hash = 0;

    // --- Event emit helpers ---
    void send_sta()
    {
        utils::send_struct_if_changed(sta_cfg, last_sta_hash, EventType::STA_ANSWER, TAG);
    }
    void send_ap()
    {
        utils::send_struct_if_changed(ap_cfg, last_ap_hash, EventType::AP_ANSWER, TAG);
    }
}
