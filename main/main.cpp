#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"
#include "sd_card.h"
#include "wifi_manager.h"
#include "esp_log.h"
#include "macro.h"

EventGroupHandle_t eg;

void worker_task(void *)
{
    ESP_LOGI("TASK", "Running worker task");
    xEventGroupSetBits(eg, 0x01); // C'est safe ici
    vTaskDelete(NULL);
}

extern "C" void app_main()
{
    INIT_NVS();

    vTaskDelay(10); // Laisse démarrer le scheduler
    eg = xEventGroupCreate();
    xTaskCreate(worker_task, "worker", 2048, nullptr, 5, nullptr);
    EventBits_t bits = xEventGroupWaitBits(eg, 0x01, pdFALSE, pdTRUE, portMAX_DELAY);
    ESP_LOGI("MAIN", "Boot OK, bits=%ld", bits);


    sd_card::start_async_init();
    while (sd_card::g_sd_status == ESP_FAIL)
    {
        // Attend (ou tu peux yield, logger, etc.)
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    if (sd_card::g_sd_status == ESP_OK)
    {
        ESP_LOGI("MAIN", "SD ready!");
        vTaskDelay(pdMS_TO_TICKS(50));

        std::vector<StaNetwork> nets;
        wifi_persistence_async::start_persist_task();

        if (wifi_persistence::load(nets))
        {
            wifi_manager::clear_sta_networks();
            for (const auto &net : nets)
                wifi_manager::add_sta_network(net.ssid, net.password);
            ESP_LOGI("MAIN", "Wifi config chargée depuis SD (%d réseaux)", (int)nets.size());
        }
        else
        {
            ESP_LOGW("MAIN", "Pas de config wifi SD, fallback réseaux par défaut");
            // Tu peux ici mettre tes add_sta_network("HaloMesh", "HaloMesh") etc. si tu veux un fallback.
        }
    }
    else
    {
        ESP_LOGE("MAIN", "SD init failed!");
    }

    wifi_manager::start_apsta_async([](wifi_manager::Status st)
                                    {
        switch(st) {
        case wifi_manager::Status::STA_CONNECTED:
            printf("[WIFI] STA CONNECTED, IP: %s\n", wifi_manager::get_sta_ip());
            break;
        case wifi_manager::Status::AP_STARTED:
            printf("[WIFI] AP UP, IP: %s\n", wifi_manager::get_ap_ip());
            break;
        case wifi_manager::Status::ALL_STA_FAILED:
            printf("[WIFI] All STA failed, AP only mode\n");
            break;
        default: break;
        } });

    vTaskDelay(pdMS_TO_TICKS(1000));
    httpd_handle_t server = NULL;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_open_sockets = 5;
    config.max_uri_handlers = 25;
    config.lru_purge_enable = true;
    config.max_resp_headers = 5;
    config.server_port = 80;

    // Start the httpd server
    if (httpd_start(&server, &config) == ESP_OK)
    {
        wifi_api::register_wifi_api_endpoints(server);

        const httpd_uri_t get_favicon_uri = {
            .uri = "/favicon.ico",
            .method = HTTP_GET,
            .handler = [](httpd_req_t *req) -> esp_err_t
            {
                httpd_resp_send(req, R"(<link rel="icon" href="data:image/svg+xml,<svg xmlns=%22http://www.w3.org/2000/svg%22 viewBox=%220 0 100 100%22><text y=%22.9em%22 font-size=%2290%22>🔥</text></svg>" type="image/svg+xml">)", HTTPD_RESP_USE_STRLEN);
                return ESP_OK;
            },
            .user_ctx = NULL};

        const httpd_uri_t error_uri = {
            .uri = "/error",
            .method = HTTP_GET,
            .handler = [](httpd_req_t *req) -> esp_err_t
            {
                httpd_resp_send(req, "Error 404", HTTPD_RESP_USE_STRLEN);
                return ESP_OK;
            },
            .user_ctx = NULL};

        // const httpd_uri_t stream_camera_uri = {
        //     .uri = "/camera_stream",
        //     .method = HTTP_GET,
        //     .handler = handle_stream,
        //     .user_ctx = NULL};

        // const httpd_uri_t capture_camera_uri = {
        //     .uri = "/camera_capture",
        //     .method = HTTP_GET,
        //     .handler = [](httpd_req_t *req) -> esp_err_t
        //     {
        //         return camera.capture_handler(req);
        //     },
        //     .user_ctx = NULL};

        ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_register_uri_handler(server, &get_favicon_uri));
        ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_register_uri_handler(server, &error_uri));
        ESP_LOGI("httpd", "Server started");
    }
    else
    {
        ESP_LOGE("httpd", "Server init failed");
    }

    xTaskCreate(wifi_manager::wifi_reconnect_async, "wifi_reconnect", 2048, nullptr, 2, nullptr);
}
