#pragma once
#ifndef __WIFI_PERSISTENCE_ASYNC_H__
#define __WIFI_PERSISTENCE_ASYNC_H__
#include <vector>
#include "wifi_manager.h"

namespace sta_persistence_async
{
    void save_task(void* arg);
    void request_save(const std::vector<net_credential_t>& nets, const char* path = "/sdcard/wifi.txt");

    // void start_persist_task();
    // void request_save(const std::vector<net_credential_t>&);
    // void request_save(const std::vector<net_credential_t>&, const char* path);
    // bool is_persisted(); // true si tout est écrit
}
#endif