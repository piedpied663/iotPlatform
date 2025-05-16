#include "wifi_api.h"
#include "wifi_manager.h"
#include "sta_persistence.h"
#include "esp_log.h"
#include <string>
namespace wifi_api
{

    static const char *TAG = "WifiAPI";
    esp_err_t wifi_sta_persisted_handler(httpd_req_t *req)
    {
        bool status = sta_persistence_async::is_persisted();
        std::string json = std::string("{\"persisted\":") + (status ? "true" : "false") + "}";
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, json.c_str(), json.size());
        return ESP_OK;
    }

    // GET /api/wifi_sta
    esp_err_t wifi_sta_get_handler(httpd_req_t *req)
    {
        const auto &nets = wifi_manager::get_sta_list();
        std::string json = sta_persistence::to_json(nets);

        httpd_resp_set_type(req, "application/json");
        return httpd_resp_send(req, json.c_str(), json.size());
    }

    esp_err_t wifi_sta_post_handler(httpd_req_t *req)
    {
        // 1) Lire tout le body
        int total = req->content_len;
        std::string body(total, '\0');
        int received = httpd_req_recv(req, &body[0], total);
        if (received <= 0)
        {
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }

        // 2) Parser en liste
        std::vector<StaNetwork> new_nets;
        if (!sta_persistence::from_json(body.c_str(), new_nets))
        {
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }

        // 3) Appliquer au manager + persister
        wifi_manager::clear_sta_networks();
        for (auto &net : new_nets)
        {
            wifi_manager::add_sta_network(net.ssid, net.password);
        }
        sta_persistence_async::request_save(wifi_manager::get_sta_list());

        httpd_resp_sendstr(req, "OK");
        return ESP_OK;
    }

    // DELETE /api/wifi_sta?ssid=xxx
    esp_err_t wifi_sta_delete_handler(httpd_req_t *req)
    {
        char query[64] = {};
        httpd_req_get_url_query_str(req, query, sizeof(query));
        char ssid_param[64];
        if (httpd_query_key_value(query, "ssid", ssid_param, sizeof(ssid_param)) == ESP_OK)
        {
            wifi_manager::remove_sta_network(ssid_param);
            sta_persistence_async::request_save(wifi_manager::get_sta_list());
            httpd_resp_sendstr(req, "OK");
            return ESP_OK;
        }
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    // Fonction pour tout enregistrer d’un coup
    void register_wifi_api_endpoints(httpd_handle_t server)
    {
        httpd_uri_t get_sta = {
            .uri = "/api/wifi_sta",
            .method = HTTP_GET,
            .handler = wifi_sta_get_handler,
            .user_ctx = NULL};
        httpd_register_uri_handler(server, &get_sta);

        httpd_uri_t post_sta = {
            .uri = "/api/wifi_sta",
            .method = HTTP_POST,
            .handler = wifi_sta_post_handler,
            .user_ctx = NULL};
        httpd_register_uri_handler(server, &post_sta);

        httpd_uri_t del_sta = {
            .uri = "/api/wifi_sta",
            .method = HTTP_DELETE,
            .handler = wifi_sta_delete_handler,
            .user_ctx = NULL};
        httpd_register_uri_handler(server, &del_sta);
    }
}