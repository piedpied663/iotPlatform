#include <stdio.h>
#include "sd_card.h"
#include "../key.h"

namespace sd_card
{
    esp_err_t g_sd_status = ESP_FAIL;
    void init_task(void *pvParameter)
    {
        ESP_LOGI(TAG, "Init SD card in task...");

        sdmmc_card_t *card;

        // Init host (SDMMC)
        sdmmc_host_t host = SDMMC_HOST_DEFAULT();
        host.flags = SDMMC_HOST_FLAG_1BIT;
        sdmmc_slot_config_t slot_cfg = SDMMC_SLOT_CONFIG_DEFAULT();
        slot_cfg.width = 1;

        esp_vfs_fat_sdmmc_mount_config_t mount_cfg = VFS_FAT_MOUNT_DEFAULT_CONFIG();
        mount_cfg.max_files = 10;
        mount_cfg.format_if_mount_failed = false;
        mount_cfg.allocation_unit_size = 16 * 1024;

        esp_err_t err = esp_vfs_fat_sdmmc_mount(
            key::SD_BASE_PATH, &host, &slot_cfg, &mount_cfg, &card);

        if (err == ESP_OK)
        {
            ESP_LOGI(TAG, "SD Card mounted: %s", card->cid.name);
        }
        else
        {
            ESP_LOGE(TAG, "SD Card mount failed: %s", esp_err_to_name(err));
        }

        g_sd_status = err;
        vTaskDelete(NULL); // Fin de la tâche
    }

    // Lance la tâche d’init (depuis app_main ou un autre service)
    void start_async_init()
    {
        xTaskCreatePinnedToCore(init_task, "SDInitTask", 4096, nullptr, 5, nullptr, tskNO_AFFINITY);
    }
}