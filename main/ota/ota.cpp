#include "ota.h"
#include "esp_log.h"
#include "esp_https_ota.h"
#include "esp_ota_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static constexpr const char* TAG = "[OTA]";

namespace ota_manager
{
    static void ota_task(void* param)
    {
        const char* url = static_cast<const char*>(param);
        esp_https_ota_config_t config = {};
        
        ESP_LOGI(TAG, "Starting OTA from: %s", url);

        esp_err_t ret = esp_https_ota(&config);
        if (ret == ESP_OK)
        {
            ESP_LOGI(TAG, "OTA successful, rebooting...");
            vTaskDelay(pdMS_TO_TICKS(1000));
            esp_restart();
        }
        else
        {
            ESP_LOGE(TAG, "OTA failed: %s", esp_err_to_name(ret));
        }

        vTaskDelete(nullptr);
    }

    void start_update(const char* url)
    {
        // On lance la tâche OTA
        xTaskCreate(ota_task, "ota_task", 8192, (void*)url, 5, nullptr);
    }

    void validate_image()
    {
        esp_err_t ret = esp_ota_mark_app_valid_cancel_rollback();
        if (ret == ESP_OK)
        {
            ESP_LOGI(TAG, "Firmware validated");
        }
        else if (ret == ESP_ERR_OTA_ROLLBACK_FAILED)
        {
            ESP_LOGW(TAG, "Rollback already done");
        }
        else
        {
            ESP_LOGE(TAG, "Error validating firmware: %s", esp_err_to_name(ret));
        }
    }
}
