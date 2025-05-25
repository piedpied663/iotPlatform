#include "api.h"
#include "esp_log.h"
#include <string>
#include "esp_http_server.h"
#include "../macro.h"

#ifdef CONFIG_IOT_FEATURE_CAMERA
#include "camera_api.h"
#endif

#include <memory>

namespace api
{
    static httpd_handle_t server = nullptr;

    // Wrapper pour émettre un event et attendre la réponse
    esp_err_t emit_event_and_wait_response(EventType req_type, EventType expected_resp_type, httpd_req_t *req)
    {
        QueueHandle_t resp_queue = xQueueCreate(1, sizeof(Event));
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
    esp_err_t mqtt_status_handler(httpd_req_t *req)
    {
        return emit_event_and_wait_response(EventType::MQTT_STATUS_REQUEST_JSON, EventType::MQTT_STATUS_ANSWER_JSON, req);
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
        private_register_endpoints(server, "/iot/wifi_ap", wifi_ap_get_handler, wifi_ap_post_handler);
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

        END_TASK(nullptr, "task_start_server");
    }

    void start_server_async()
    {
        xTaskCreate(task_start_server, "httpd_async", STACK_SIZE_HTTPD, nullptr, tskIDLE_PRIORITY + 1, nullptr);
    }

    void on_event(const Event &evt)
    {
        if (evt.type == EventType::HTTPD_START && !_server_started)
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
