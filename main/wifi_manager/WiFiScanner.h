#pragma once
#ifndef __WIFI_SCANNER_H__
#define __WIFI_SCANNER_H__
#include <vector>
#include "event_bus.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "types.h"

class WiFiScanner
{
public:
    static constexpr const char *TAG = "[WFS]";
    static WiFiScanner &getInstance();


private:
    static void on_event(const Event *evt);
    static void on_wifi_event(void *arg, esp_event_base_t base, int32_t id, void *data);
    WiFiScanner() {};
    WiFiScanner(const WiFiScanner &) = delete;
    WiFiScanner &operator=(const WiFiScanner &) = delete;

    struct register_event_bus
    {
        register_event_bus()
        {
            EventBus::getInstance().subscribe(on_event, TAG);
            // esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_SCAN_DONE, &on_wifi_event, nullptr);
        }
    };

    inline static register_event_bus _register_event_bus;
};

#endif