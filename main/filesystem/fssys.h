#pragma once
#ifndef __FSSYS_H__
#define __FSSYS_H_
#include "event_bus.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_littlefs.h"

namespace fssys
{
    static constexpr const char *TAG = "[FSSYS]";

    void mount_task(void *);

    extern void on_event(const Event &evt);
    static bool _subscribed = false;
    struct register_event_bus
    {
        register_event_bus()
        {
            if (!_subscribed)
            {
                EventBus::getInstance().subscribe(on_event, TAG);
                _subscribed = true;
            }
        }
    };

    static register_event_bus reg_event_bus;

}

#endif