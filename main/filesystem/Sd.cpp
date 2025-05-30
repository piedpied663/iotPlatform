#include "Sd.h"
#include "freeRTOS/FreeRTOS.h"
#include "freeRTOS/task.h"

IotSD &IotSD::getInstance()
{
    static IotSD instance;
    return instance;
}

void IotSD::mount()
{
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
        "/sd", &host, &slot_cfg, &mount_cfg, &card);

    Event evt = {};
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "SD Card mounted: %s", card->cid.name);
        _mounted = true;
        evt.type = EventType::SD_READY;
    }
    else
    {
        ESP_LOGE(TAG, "SD Card mount failed: %s", esp_err_to_name(err));
        evt.type = EventType::SD_ERROR;
    }
    EventBus::getInstance().emit(evt);
}

static void mount_async(void *arg)
{
    IotSD::getInstance().mount();
    vTaskDelete(nullptr);
}

void IotSD::on_event(const Event *evt)
{
    if (evt->type == EventType::BOOT_CAN_START)
    {
        xTaskCreate(mount_async, "fs_mount", 4096, nullptr, 5, nullptr);
    }
}
