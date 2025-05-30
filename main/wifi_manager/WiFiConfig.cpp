#include "WiFiConfig.h"
#include "esp_log.h"
#include "cJSON.h"
#include "persistence.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "utils.h"
#include "macro.h"
// void WiFiConfig::emitEvent(EventType type, void *user_ctx)
// {
//     Event e{};
//     e.type = type;
//     if (user_ctx != nullptr)
//     {
//         e.user_ctx = user_ctx;
//     }

//     EventBus::getInstance().emit(e);
// }
static void async_load_sta(void *p)
{
    if (WiFiConfig::getInstance().load_sta())
    {
        utils::emitEvent(EventType::STA_LOAD_SUCCESS, WiFiConfig::getInstance().get_sta());
        ESP_LOGI(WiFiConfig::TAG, "STA.bin loaded.");
    }
    else
    {
        utils::emitEvent(EventType::STA_LOAD_FAIL, WiFiConfig::getInstance().get_sta());
        ESP_LOGW(WiFiConfig::TAG, "No STA.bin using default.");
    }

    vTaskDelete(NULL);
}

static void async_load_ap(void *p)
{
    if (WiFiConfig::getInstance().load_ap())
    {
        utils::emitEvent(EventType::AP_LOAD_SUCCESS, WiFiConfig::getInstance().get_ap());
        ESP_LOGI(WiFiConfig::TAG, "AP.bin loaded.");
    }
    else
    {
        utils::emitEvent(EventType::AP_LOAD_FAIL, WiFiConfig::getInstance().get_ap());
        ESP_LOGW(WiFiConfig::TAG, "No AP.bin using default.");
    }
    vTaskDelete(NULL);
}

void WiFiConfig::task_load_sta()
{
    xTaskCreate(async_load_sta, "load_sta", 4096, NULL, tskIDLE_PRIORITY + 1, NULL);
}
static void save_sta(void *)
{
    auto sta_cfg = WiFiConfig::getInstance().get_sta();
    persist::save_struct(STA_CFG_PATH, &sta_cfg, sizeof(sta_cfg));
}
void WiFiConfig::task_save_sta()
{
    xTaskCreate(save_sta, "save_sta", 4096, NULL, tskIDLE_PRIORITY + 1, NULL);
}

static void save_ap(void *)
{
    auto ap_cfg = WiFiConfig::getInstance().get_ap();
    persist::save_struct(AP_CFG_PATH, &ap_cfg, sizeof(ap_cfg));
}

void WiFiConfig::task_save_ap()
{
    xTaskCreate(save_ap, "save_ap", 4096, NULL, tskIDLE_PRIORITY + 1, NULL);
}

void WiFiConfig::task_load_ap()
{
    xTaskCreate(async_load_ap, "load_ap", 4096, NULL, tskIDLE_PRIORITY + 1, NULL);
}

bool WiFiConfig::load_sta()
{
    return persist::load_struct(STA_CFG_PATH, &sta_cfg, sizeof(sta_cfg));
}

bool WiFiConfig::load_ap(void)
{
    return persist::load_struct(AP_CFG_PATH, &ap_cfg, sizeof(ap_cfg));
}

void WiFiConfig::update_sta(const net_sta_config_t *cfg)
{
    memcpy(&sta_cfg, cfg, sizeof(*cfg));
    utils::emitEvent(EventType::STA_CHANGED, &sta_cfg);
}

void WiFiConfig::update_ap(const net_ap_config_t *cfg)
{
    memcpy(&ap_cfg, cfg, sizeof(*cfg));
    utils::emitEvent(EventType::AP_CHANGED, &ap_cfg);
}

bool WiFiConfig::sta_to_json(char *out_buf, size_t buf_len)
{
    auto sta_cfg = WiFiConfig::getInstance().get_sta();

    cJSON *root = cJSON_CreateObject();
    cJSON *arr = cJSON_AddArrayToObject(root, "stations");
    for (size_t i = 0; i < sta_cfg->count; ++i)
    {
        cJSON *obj = cJSON_CreateObject();
        cJSON_AddStringToObject(obj, "ssid", sta_cfg->credentials[i].ssid);
        cJSON_AddStringToObject(obj, "password", sta_cfg->credentials[i].password);
        cJSON_AddItemToArray(arr, obj);
    }
    cJSON_AddNumberToObject(root, "count", sta_cfg->count);
    cJSON_AddNumberToObject(root, "currentNetwork", sta_cfg->currentNetwork);
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

bool WiFiConfig::sta_from_json(const char *json, net_sta_config_t *cfg)
{
    cJSON *root = cJSON_Parse(json);
    if (!root)
        return false;
    cJSON *arr = cJSON_GetObjectItemCaseSensitive(root, "stations");
    if (!cJSON_IsArray(arr))
    {
        cJSON_Delete(root);
        return false;
    }
    cfg->count = 0;
    cJSON *item;
    cJSON_ArrayForEach(item, arr)
    {
        if (cfg->count >= WIFI_MAX_NETWORKS)
            break;
        cJSON *j_ssid = cJSON_GetObjectItemCaseSensitive(item, "ssid");
        cJSON *j_pwd = cJSON_GetObjectItemCaseSensitive(item, "password");
        if (cJSON_IsString(j_ssid) && cJSON_IsString(j_pwd))
        {
            strncpy(cfg->credentials[cfg->count].ssid, j_ssid->valuestring, WIFI_MAX_SSID_LEN - 1);
            strncpy(cfg->credentials[cfg->count].password, j_pwd->valuestring, WIFI_MAX_PASS_LEN - 1);
            cfg->credentials[cfg->count].ssid[WIFI_MAX_SSID_LEN - 1] = 0;
            cfg->credentials[cfg->count].password[WIFI_MAX_PASS_LEN - 1] = 0;
            cfg->count++;
        }
    }
    cJSON_Delete(root);
    return true;
}

bool WiFiConfig::ap_to_json(char *out_buf, size_t buf_len)
{
    auto apcfg = WiFiConfig::getInstance().get_ap();

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "ssid", apcfg->ssid);
    cJSON_AddStringToObject(root, "password", apcfg->password);
    cJSON_AddNumberToObject(root, "channel", apcfg->channel);
    cJSON_AddNumberToObject(root, "max_connection", apcfg->max_connection);

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

bool WiFiConfig::ap_from_json(const char *json, net_ap_config_t *cfg)
{
    cJSON *root = cJSON_Parse(json);
    if (!root)
        return false;
    cJSON *j_ssid = cJSON_GetObjectItemCaseSensitive(root, "ssid");
    cJSON *j_pwd = cJSON_GetObjectItemCaseSensitive(root, "password");
    cJSON *j_chn = cJSON_GetObjectItemCaseSensitive(root, "channel");
    cJSON *j_max = cJSON_GetObjectItemCaseSensitive(root, "max_connection");

    if (cJSON_IsString(j_ssid) && cJSON_IsString(j_pwd) && cJSON_IsNumber(j_chn) && cJSON_IsNumber(j_max))
    {
        strncpy(cfg->ssid, j_ssid->valuestring, WIFI_MAX_SSID_LEN - 1);
        strncpy(cfg->password, j_pwd->valuestring, WIFI_MAX_PASS_LEN - 1);
        cfg->ssid[WIFI_MAX_SSID_LEN - 1] = 0;
        cfg->password[WIFI_MAX_PASS_LEN - 1] = 0;
        cfg->channel = (uint8_t)j_chn->valuedouble;
        cfg->max_connection = (uint8_t)j_max->valuedouble;
        cJSON_Delete(root);
        return true;
    }
    cJSON_Delete(root);
    return false;
}

void WiFiConfig::on_event(const Event *evt)
{

    if (evt->type == EventType::FS_READY)
    {
        ESP_LOGI(TAG, "WiFiEvent FS_READY");
        WiFiConfig::getInstance().task_load_ap();
        WiFiConfig::getInstance().task_load_sta();
    }
    else if (evt->type == EventType::STA_REQUEST_JSON)
    {
        Event e = {};
        e.type = EventType::STA_ANSWER_JSON;
        char json[512];
        sta_to_json(json, sizeof(json));
        BIND_DATA_EVENT_JSON(e, json);
        xQueueSend((QueueHandle_t)evt->user_ctx, &e, 0);
    }
    else if (evt->type == EventType::AP_REQUEST_JSON)
    {
        if (evt->user_ctx)
        {
            Event e = {};
            e.type = EventType::AP_ANSWER_JSON;
            char json[512];
                        
            ap_to_json(json, sizeof(json));
            BIND_DATA_EVENT_JSON(e, json);
            xQueueSend((QueueHandle_t)evt->user_ctx, &e, 0);
        }
    }
    else if (evt->type == EventType::STA_POST_REQUEST)
    {
        if (evt->user_ctx)
        {
            const char *json = reinterpret_cast<const char *>(evt->data);
            auto sta_cfg = WiFiConfig::getInstance().get_sta();

            if (sta_from_json(json, sta_cfg))
            {
                WiFiConfig::getInstance().task_save_sta();
            }
            Event resp_evt{};
            BIND_DATA_EVENT_JSON(resp_evt, json);
            resp_evt.type = EventType::STA_POST_ANSWER;
            xQueueSend((QueueHandle_t)evt->user_ctx, &resp_evt, 0);
        }
    }
    else if (evt->type == EventType::AP_POST_REQUEST)
    {
        if (evt->user_ctx)
        {
            char json[512];
            if (WiFiConfig::getInstance().ap_to_json(json, sizeof(json)))
            {
                WiFiConfig::getInstance().task_save_ap();
            }
            Event resp_evt = {};
            resp_evt.type = EventType::AP_ANSWER_JSON;
            BIND_DATA_EVENT_JSON(resp_evt, json);
            // Envoie la réponse
            xQueueSend((QueueHandle_t)evt->user_ctx, &resp_evt, 0);
        }
    }
}
