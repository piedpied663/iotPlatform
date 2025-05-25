#pragma once
#ifndef __WIFI_API_H__
#define __WIFI_API_H__
#include "esp_http_server.h"
#include "event_bus.h"
#include "macro.h"

namespace api
{
    static constexpr const char *TAG = "[API]";
    esp_err_t emit_event_and_wait_response(EventType req_type, EventType expected_resp_type, httpd_req_t *req);
    esp_err_t handle_post_request(EventType post_req_type, EventType expected_resp_type, httpd_req_t *req);

    esp_err_t wifi_sta_get_handler(httpd_req_t *req);
    esp_err_t wifi_sta_post_handler(httpd_req_t *req);

    esp_err_t wifi_ap_get_handler(httpd_req_t *req);
    esp_err_t wifi_ap_post_handler(httpd_req_t *req);
    // esp_err_t wifi_ap_delete_handler(httpd_req_t *req);
    // esp_err_t wifi_sta_delete_handler(httpd_req_t *req);
    esp_err_t mqtt_get_handler(httpd_req_t *req);
    esp_err_t mqtt_post_handler(httpd_req_t *req);
    void private_register_endpoints(httpd_handle_t server, const char *base_path,
                                    http_handler_t get_handler,
                                    http_handler_t post_handler);
    // Appelle cette fonction dans ton main pour tout enregistrer d’un coup
    void register_wifi_api_endpoints(httpd_handle_t server);

    void task_start_server(void *);

    void start_server_async();

    void on_event(const Event &evt);

    httpd_handle_t get_server();
    static bool subscribed = false;
    struct register_event_bus
    {
        register_event_bus()
        {
            if (!subscribed)
            {
                EventBus::getInstance().subscribe(on_event, TAG);
                subscribed = true;
            }
        }
    };

    inline static register_event_bus reg;

}

#endif
