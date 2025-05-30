#include "api.h"
#include "esp_log.h"
#include <string>
#include "esp_http_server.h"
#include "../macro.h"

#ifdef CONFIG_IOT_FEATURE_CAMERA
#include "camera_api.h"
#endif

#include <memory>
#include "types.h"
#include "utils.h"
#include "ota.h"
namespace api
{
    static httpd_handle_t server = nullptr;

    // Wrapper pour émettre un event et attendre la réponse
    esp_err_t emit_event_and_wait_response(EventType req_type, EventType expected_resp_type, httpd_req_t *req)
    {
        QueueHandle_t resp_queue = xQueueCreate(2, sizeof(Event));
        if (!resp_queue)
        {
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }

        Event evt = {};
        evt.type = req_type;
        evt.user_ctx = resp_queue;

        EventBus::getInstance().emit(evt);

        Event resp_evt;
        if (xQueueReceive(resp_queue, &resp_evt, pdMS_TO_TICKS(200)) == pdTRUE &&
            (expected_resp_type == EventType::NONE || resp_evt.type == expected_resp_type))
        {
            SET_RESP_HEADERS(req);
            httpd_resp_send(req, (const char *)resp_evt.data, resp_evt.data_len);
            vQueueDelete(resp_queue);
            return ESP_OK;
        }
        else
        {
            vQueueDelete(resp_queue);
            return httpd_resp_send_500(req);
        }
    }

    // Wrapper pour lire le corps HTTP, émettre un event, et attendre la réponse
    esp_err_t handle_post_request(EventType post_req_type, EventType expected_resp_type, httpd_req_t *req)
    {
        size_t len = req->content_len;
        if (len == 0)
        {
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }

        std::unique_ptr<char[]> buf(new char[len + 1]);
        if (httpd_req_recv(req, buf.get(), len) != (ssize_t)len)
        {
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        buf[len] = '\0';

        QueueHandle_t resp_queue = xQueueCreate(1, sizeof(Event));
        if (!resp_queue)
        {
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }

        Event evt{};
        evt.type = post_req_type;
        evt.user_ctx = resp_queue;
        evt.data_len = len;
        memcpy(evt.data, buf.get(), len);

        EventBus::getInstance().emit(evt);

        Event answer_evt;
        if (xQueueReceive(resp_queue, &answer_evt, pdMS_TO_TICKS(200)) == pdTRUE &&
            answer_evt.type == expected_resp_type)
        {
            SET_RESP_HEADERS(req);
            httpd_resp_send(req, (const char *)answer_evt.data, answer_evt.data_len);
            vQueueDelete(resp_queue);
            return ESP_OK;
        }
        else
        {
            vQueueDelete(resp_queue);
            return httpd_resp_send_500(req);
        }
    }

    // Handlers GET
    esp_err_t wifi_sta_get_handler(httpd_req_t *req)
    {
        return emit_event_and_wait_response(EventType::STA_REQUEST_JSON, EventType::STA_ANSWER_JSON, req);
    }

    esp_err_t wifi_ap_get_handler(httpd_req_t *req)
    {
        return emit_event_and_wait_response(EventType::AP_REQUEST_JSON, EventType::AP_ANSWER_JSON, req);
    }

    esp_err_t mqtt_get_handler(httpd_req_t *req)
    {
        return emit_event_and_wait_response(EventType::MQTT_CONFIG_REQUEST_JSON, EventType::MQTT_CONFIG_ANSWER_JSON, req);
    }

    // Handlers POST
    esp_err_t wifi_sta_post_handler(httpd_req_t *req)
    {
        return handle_post_request(EventType::STA_POST_REQUEST, EventType::STA_POST_ANSWER, req);
    }

    esp_err_t wifi_ap_post_handler(httpd_req_t *req)
    {
        return handle_post_request(EventType::AP_POST_REQUEST, EventType::AP_POST_ANSWER, req);
    }

    esp_err_t wifi_status_handler(httpd_req_t *req)
    {
        return emit_event_and_wait_response(EventType::WIFI_STATUS_REQUEST_JSON, EventType::WIFI_STATUS_ANSWER_JSON, req);
    }

    esp_err_t mqtt_post_handler(httpd_req_t *req)
    {
        return handle_post_request(EventType::MQTT_POST_REQUEST, EventType::MQTT_POST_ANSWER, req);
    }
    esp_err_t fs_list_handler(httpd_req_t *req)
    {
        return emit_event_and_wait_response(EventType::FS_LIST_FILES_REQUEST, EventType::FS_LIST_FILES_ANSWER, req);
    }
    esp_err_t mqtt_status_handler(httpd_req_t *req)
    {
        return emit_event_and_wait_response(EventType::MQTT_STATUS_REQUEST_JSON, EventType::MQTT_STATUS_ANSWER_JSON, req);
    }
    esp_err_t wifi_scan_handler(httpd_req_t *req)
    {
        QueueHandle_t resp_queue = xQueueCreate(2, sizeof(Event));
        if (!resp_queue)
        {
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }

        Event evt = {};
        evt.type = EventType::WIFI_SCAN_REQUEST;
        evt.user_ctx = resp_queue;

        EventBus::getInstance().emit(evt);

        Event resp_evt;
        if (xQueueReceive(resp_queue, &resp_evt, pdMS_TO_TICKS(5000)) == pdTRUE &&
            resp_evt.type == EventType::WIFI_SCAN_RESULT)
        {
            // Ici tu reconstruis ton JSON depuis resp_evt.data
            // const scan_result_t *result = reinterpret_cast<const scan_result_t *>(resp_evt.data);
            scan_result_t result;
            memcpy(&result, resp_evt.data, sizeof(result));

            cJSON *root = cJSON_CreateObject();
            if (!root)
            {
                ESP_LOGE(TAG, "cJSON_CreateObject failed");
                vQueueDelete(resp_queue);
                return httpd_resp_send_500(req);
            }

            cJSON *array = cJSON_CreateArray();
            if (!array)
            {
                ESP_LOGE(TAG, "cJSON_CreateArray failed");
                cJSON_Delete(root);
                vQueueDelete(resp_queue);
                return httpd_resp_send_500(req);
            }
            uint8_t final_count = 0;
            // for (int i = 0; i < result.count; ++i)
            // {
            //     if (i >= MAX_SCAN_RESULTS)
            //     {
            //         break;
            //     }
            //     wifi_ap_record_t r = result.list[i];
            //     cJSON *item = cJSON_CreateObject();
            //     char ssid_buf[sizeof(r.ssid) + 1] = {0};
            //     memcpy(ssid_buf, r.ssid, sizeof(r.ssid));
            //     ssid_buf[sizeof(r.ssid)] = '\0';
            //     if(strlen(ssid_buf) == 0) continue;
            //     if(ssid_buf[0] == '\0') continue;
            //     if(strcmp(ssid_buf, "") == 0) continue;

            //     cJSON_AddStringToObject(item, "ssid", ssid_buf);
            //     cJSON_AddNumberToObject(item, "rssi", r.rssi);
            //     cJSON_AddNumberToObject(item, "channel", r.primary);
            //     cJSON_AddNumberToObject(item, "authmode", r.authmode);
            //     cJSON_AddItemToArray(array, item);
            //     final_count++;
            // }
            for (int i = 0; i < result.count; ++i)
            {
                if (i >= MAX_SCAN_RESULTS)
                    break;

                wifi_ap_record_t r = result.list[i];
                cJSON *item = cJSON_CreateObject();
                if (!item)
                    continue;

                // Safe SSID extraction
                char ssid_buf[sizeof(r.ssid) + 1] = {0};
                memcpy(ssid_buf, r.ssid, sizeof(r.ssid));
                ssid_buf[sizeof(r.ssid)] = '\0';

                if (ssid_buf[0] == '\0')
                    continue;

                cJSON_AddStringToObject(item, "ssid", ssid_buf);
                cJSON_AddNumberToObject(item, "rssi", r.rssi);
                cJSON_AddNumberToObject(item, "channel", r.primary);
                cJSON_AddNumberToObject(item, "authmode", r.authmode);
                cJSON_AddStringToObject(item, "bssid", utils::mac_to_string(r.bssid).c_str());
                cJSON_AddNumberToObject(item, "pairwise_cipher", r.pairwise_cipher);
                cJSON_AddNumberToObject(item, "group_cipher", r.group_cipher);
                cJSON_AddNumberToObject(item, "bandwidth", r.bandwidth);
                cJSON_AddNumberToObject(item, "vht_ch_freq1", r.vht_ch_freq1);
                cJSON_AddNumberToObject(item, "vht_ch_freq2", r.vht_ch_freq2);

                // Flags PHY (packés sur des bits, on les remplit un par un)
                cJSON *phy = cJSON_CreateObject();
                if (phy)
                {
                    cJSON_AddBoolToObject(phy, "11b", r.phy_11b);
                    cJSON_AddBoolToObject(phy, "11g", r.phy_11g);
                    cJSON_AddBoolToObject(phy, "11n", r.phy_11n);
                    cJSON_AddBoolToObject(phy, "lr", r.phy_lr);
                    cJSON_AddBoolToObject(phy, "11a", r.phy_11a);
                    cJSON_AddBoolToObject(phy, "11ac", r.phy_11ac);
                    cJSON_AddBoolToObject(phy, "11ax", r.phy_11ax);
                    cJSON_AddItemToObject(item, "phy", phy);
                }

                // Flags divers
                cJSON_AddBoolToObject(item, "wps", r.wps);
                cJSON_AddBoolToObject(item, "ftm_responder", r.ftm_responder);
                cJSON_AddBoolToObject(item, "ftm_initiator", r.ftm_initiator);

                // (optionnel) tu pourrais aussi rajouter le country ici si besoin

                cJSON_AddItemToArray(array, item);
                final_count++;
            }
            cJSON_AddNumberToObject(root, "count", final_count);
            cJSON_AddItemToObject(root, "results", array);

            char *json_str = cJSON_PrintUnformatted(root);
            cJSON_Delete(root);
            SET_RESP_HEADERS(req);
            httpd_resp_send(req, json_str, strlen(json_str));
            cJSON_free(json_str);
            vQueueDelete(resp_queue);
            return ESP_OK;
        }
        else
        {
            vQueueDelete(resp_queue);
            return httpd_resp_send_500(req);
        }
    }

    esp_err_t ota_post_handler(httpd_req_t *req)
    {
        char buf[512] = {};
        int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
        if (ret <= 0)
        {
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }

        buf[ret] = '\0';

        ESP_LOGI(TAG, "Received OTA URL: %s", buf);

        // Lancer OTA
        ota_manager::start_update(buf);

        httpd_resp_send(req, nullptr, 0);
        return ESP_OK;
    }

    // Fonction d'enregistrement générique des endpoints HTTP
    void private_register_endpoints(httpd_handle_t server, const char *base_path,
                                    http_handler_t get_handler,
                                    http_handler_t post_handler)
    {
        if (!server)
            return;

        if (get_handler)
        {
            httpd_uri_t get_uri = {
                .uri = base_path,
                .method = HTTP_GET,
                .handler = get_handler,
                .user_ctx = nullptr};

            ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_register_uri_handler(server, &get_uri));
        }
        if (post_handler)
        {
            httpd_uri_t post_uri = {
                .uri = base_path,
                .method = HTTP_POST,
                .handler = post_handler,
                .user_ctx = nullptr};

            ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_register_uri_handler(server, &post_uri));
        }
        if (get_handler || post_handler)
        {

            httpd_uri_t options_uri = {
                .uri = base_path,
                .method = HTTP_OPTIONS,
                .handler = [](httpd_req_t *req)
                {
                    SET_CORS_HEADERS(req);
                    HANDLE_OPTIONS(req);
                    return ESP_OK;
                },
                .user_ctx = nullptr};

            ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_register_uri_handler(server, &options_uri));
        }
    }

    void register_wifi_api_endpoints(httpd_handle_t server)
    {
        private_register_endpoints(server, "/iot/mqtt", mqtt_get_handler, mqtt_post_handler);
        private_register_endpoints(server, "/iot/mqtt_status", mqtt_status_handler, nullptr);
        private_register_endpoints(server, "/iot/wifi_sta", wifi_sta_get_handler, wifi_sta_post_handler);
        private_register_endpoints(server, "/iot/wifi_status", wifi_status_handler, nullptr);
        private_register_endpoints(server, "/iot/wifi_scan", wifi_scan_handler, nullptr);
        private_register_endpoints(server, "/iot/wifi_ap", wifi_ap_get_handler, wifi_ap_post_handler);
        private_register_endpoints(server, "/iot/fs", fs_list_handler, nullptr);
        private_register_endpoints(server, "/iot/ota", nullptr, ota_post_handler);
        private_register_endpoints(server, "/iot/ping", [](httpd_req_t *req)
                                   {
                SET_RESP_HEADERS(req);
                return httpd_resp_send(req, nullptr, 0); }, nullptr);

#ifdef CONFIG_IOT_FEATURE_CAMERA
        private_register_endpoints(server, "/iot/camera", camera_api::camera_get, camera_api::camera_post);
        private_register_endpoints(server, "/camera_save", nullptr, camera_api::camera_save);
#endif
    }

    static bool _server_started = false;
#define STACK_SIZE_HTTPD 8192
    void task_start_server(void *)
    {
        httpd_config_t config = HTTPD_DEFAULT_CONFIG();
        config.max_open_sockets = 5;
        config.max_uri_handlers = 50;
        config.lru_purge_enable = true;
        config.max_resp_headers = 5;
        config.server_port = 80;
        config.stack_size = STACK_SIZE_HTTPD;

        if (httpd_start(&server, &config) == ESP_OK)
        {
            register_wifi_api_endpoints(server);
        }

        vTaskDelete(nullptr);
    }

    void start_server_async()
    {
        xTaskCreate(task_start_server, "httpd_async", STACK_SIZE_HTTPD, nullptr, tskIDLE_PRIORITY + 1, nullptr);
    }

    void on_event(const Event *evt)
    {
        if (evt->type == EventType::HTTPD_START && !_server_started)
        {
            start_server_async();
            _server_started = true;
            Event e = {};
            e.type = EventType::NONE;
            EventBus::getInstance().emit(e);
        }
    }
    httpd_handle_t get_server()
    {
        return server;
    }
}
