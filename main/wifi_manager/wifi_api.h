#pragma once
#ifndef __WIFI_API_H__
#define __WIFI_API_H__
#include "esp_http_server.h"

namespace wifi_api
{
    esp_err_t wifi_sta_get_handler(httpd_req_t *req);
    esp_err_t wifi_sta_post_handler(httpd_req_t *req);
    esp_err_t wifi_sta_delete_handler(httpd_req_t *req);

    esp_err_t wifi_ap_get_handler(httpd_req_t *req);
    esp_err_t wifi_ap_post_handler(httpd_req_t *req);
    esp_err_t wifi_ap_delete_handler(httpd_req_t *req);


    // Appelle cette fonction dans ton main pour tout enregistrer d’un coup
    void register_wifi_api_endpoints(httpd_handle_t server);
}

#endif
