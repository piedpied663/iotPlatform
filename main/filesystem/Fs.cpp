#include "Fs.h"
static bool _little_mounted = false;

static void mount_littlefs(void *)
{
    esp_vfs_littlefs_conf_t conf = {
#ifdef CONFIG_IOT_FEATURE_SD
        .base_path = "/sd",
#else
        .base_path = "/fs",
#endif
        .partition_label = "storage",
        .format_if_mount_failed = true,
        .dont_mount = false,
    };
    esp_err_t res = esp_vfs_littlefs_register(&conf);
    Event evt = {};
    if (res == ESP_OK)
    {
        _little_mounted = true;
        evt.type = EventType::FS_READY;
    }
    else
    {
        evt.type = EventType::FS_ERROR;
    }

    EventBus::getInstance().emit(evt);
    vTaskDelete(nullptr);
}

static void mount_dual(void *)
{
#ifdef CONFIG_IOT_FEATURE_SD
    utils::emitEvent(EventType::SD_MOUNT); // send signal to sd mount task
#endif
    vTaskDelete(nullptr);
}

void FS::on_event(const Event *evt)
{
    if (evt->type == EventType::BOOT_CAN_START)
    {
#ifdef CONFIG_IOT_FEATURE_SD
        xTaskCreate(mount_dual, "fs_mount", 4096, nullptr, 5, nullptr);
#else
        xTaskCreate(mount_littlefs, "fs_mount", 4096, nullptr, 5, nullptr);
#endif
    }
    else if (evt->type == EventType::SD_READY) // receive signal from sd mount task
    {
        utils::emitEvent(EventType::FS_READY); //  send signal to ready
    }
    else if (evt->type == EventType::SD_ERROR) // receive signal from sd mount task
    {
        xTaskCreate(mount_littlefs, "fs_mount", 4096, nullptr, 5, nullptr);
    }
}