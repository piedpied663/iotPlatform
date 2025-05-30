#pragma once
#ifndef __FS_H__
#define __FS_H__

#include "event_bus.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_littlefs.h"

#ifdef CONFIG_IOT_FEATURE_SD
#include "Sd.h"
#endif

class FS final
{
public:
    static constexpr const char *TAG = "[FS]";
    static void on_event(const Event *evt);
    struct register_event_bus
    {
        register_event_bus()
        {
            EventBus::getInstance().subscribe(on_event, TAG);
        }
    };
    inline static register_event_bus _register_event_bus;
    FS() {};
    FS(const FS &other) = delete;
    FS &operator=(const FS &other) = delete;
    static FS &getInstance()
    {
        static FS instance;
        return instance;
    }

        
};

#endif