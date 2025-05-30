#pragma once

#include "WiFiScanner.h"
#include "esp_log.h"
#include "macro.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "event_bus.h"
#include <cstring>
#include "types.h"
#include "utils.h"

static constexpr const char* TAG = "[WFS]";

// structure des résultats
static scan_result_t last_scan = {};

WiFiScanner& WiFiScanner::getInstance()
{
    static WiFiScanner instance;
    return instance;
}

static void wifi_scan_task(void* arg)
{
    QueueHandle_t response_queue = static_cast<QueueHandle_t>(arg);

    wifi_scan_config_t config = {};
    config.show_hidden = true;
    config.scan_type = WIFI_SCAN_TYPE_ACTIVE;

    // Scan bloquant
    esp_err_t err = esp_wifi_scan_start(&config, true);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Scan start failed: %s", esp_err_to_name(err));
        vTaskDelete(nullptr);
        return;
    }

    uint16_t total = 0;
    esp_wifi_scan_get_ap_num(&total);
    uint16_t to_fetch = (total > MAX_SCAN_RESULTS) ? MAX_SCAN_RESULTS : total;

    esp_wifi_scan_get_ap_records(&to_fetch, last_scan.list);
    last_scan.count = to_fetch;

    ESP_LOGI(TAG, "Scan done, found %d APs", last_scan.count);

    Event resp_evt = {};
    resp_evt.type = EventType::WIFI_SCAN_RESULT;
    memcpy(resp_evt.data, &last_scan, sizeof(last_scan));
    resp_evt.data_len = sizeof(last_scan);

    xQueueSend(response_queue, &resp_evt, portMAX_DELAY);

    vTaskDelete(nullptr);
}

void WiFiScanner::on_event(const Event* evt)
{
    if (evt->type == EventType::WIFI_SCAN_REQUEST)
    {
        xTaskCreate(wifi_scan_task, "wifi_scan_task", 8192, evt->user_ctx, tskIDLE_PRIORITY + 1, nullptr);
    }
}
