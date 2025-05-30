#pragma once
#ifndef __WIFI_MANAGER_H__
#define __WIFI_MANAGER_H__

#include <cstddef>
#include <cstdint>
#include "event_bus.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "mdns.h"
#include "lwip/lwip_napt.h"
#include "lwip/dns.h"
#include <cstring>
#include "types.h"
#include "event_bus.h"

#define WIFI_MAX_NETWORKS 5
#define WIFI_MAX_SSID_LEN 32
#define WIFI_MAX_PASS_LEN 64

class WiFiManager
{
public:
    static constexpr const char *TAG = "[WFMANG]";

private:
    static void on_event(const Event *evt);
    struct register_event_bus
    {
        register_event_bus()
        {
            EventBus::getInstance().subscribe(on_event, TAG);
        }
    };
    inline static register_event_bus _register_event_bus;
    WiFiManager() {}

public:
    static WiFiManager &getInstance();

    WiFiManager(const WiFiManager &other) = delete;
    WiFiManager &operator=(const WiFiManager &other) = delete;

private:
    void cleanup_netifs();
    void connect_next_sta();
    void task_connect_wifi();
    void start_apsta_async();
    static void on_wifi_event(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);


};

#endif