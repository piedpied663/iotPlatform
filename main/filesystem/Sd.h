#pragma once
#ifndef __SD_H__
#define __SD_H__
// #ifdef CONFIG_IOT_FEATURE_SD

#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_default_configs.h"
#include "utils.h"
#include "event_bus.h"

class IotSD
{
public:
    static constexpr const char *TAG = "[SD]";
    bool _mounted = false;
    IotSD() {}
    IotSD(const IotSD &) = delete;
    IotSD &operator=(const IotSD &) = delete;

    static IotSD &getInstance();
    void mount();
    static void on_event(const Event *evt);
    struct register_event_bus
    {
        register_event_bus()
        {
            EventBus::getInstance().subscribe(on_event, TAG);
        }
    };
    inline static register_event_bus _register_event_bus;

};
#endif