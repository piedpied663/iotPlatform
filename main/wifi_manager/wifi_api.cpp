#include "wifi_api.h"
#include "wifi_manager.h"
#include "wifi_persistence.h"
#include "esp_log.h"
#include <string>
namespace wifi_api
{

    static const char *TAG = "WifiAPI";
    esp_err_t wifi_sta_persisted_handler(httpd_req_t *req)
    {
        bool status = wifi_persistence_async::is_persisted();
        std::string json = std::string("{\"persisted\":") + (status ? "true" : "false") + "}";
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, json.c_str(), json.size());
        return ESP_OK;
    }

    // GET /api/wifi_sta
    esp_err_t wifi_sta_get_handler(httpd_req_t *req)
    {
        const auto &nets = wifi_manager::get_sta_list();
        std::string json = "[";
        for (size_t i = 0; i < nets.size(); ++i)
        {
            json += "{\"ssid\":\"" + nets[i].ssid + "\",\"password\":\"" + nets[i].password + "\"}";
            if (i + 1 < nets.size())
                json += ",";
        }
        json += "]";
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, json.c_str(), json.size());
        return ESP_OK;
    }

// POST /api/wifi_sta
#include "cJSON.h"
    // ... reste des includes

    esp_err_t wifi_sta_post_handler(httpd_req_t *req)
    {
        char buf[256] = {};
        int len = httpd_req_recv(req, buf, sizeof(buf) - 1);
        if (len <= 0)
        {
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }

        cJSON *root = cJSON_Parse(buf);
        if (!root)
        {
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }

        cJSON *j_ssid = cJSON_GetObjectItem(root, "ssid");
        cJSON *j_pwd = cJSON_GetObjectItem(root, "password");
        if (!cJSON_IsString(j_ssid) || !cJSON_IsString(j_pwd) ||
            strlen(j_ssid->valuestring) == 0 || strlen(j_ssid->valuestring) > 32)
        {
            cJSON_Delete(root);
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }

        std::string ssid = j_ssid->valuestring;
        std::string pass = j_pwd->valuestring ? j_pwd->valuestring : "";

        cJSON_Delete(root);

        // Anti-doublon + validité côté manager :
        if (!wifi_manager::add_sta_network(ssid, pass))
        {
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }

        wifi_persistence_async::request_save(wifi_manager::get_sta_list());
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
            wifi_persistence_async::request_save(wifi_manager::get_sta_list());
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