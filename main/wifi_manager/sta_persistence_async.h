#pragma once
#ifndef __WIFI_PERSISTENCE_ASYNC_H__
#define __WIFI_PERSISTENCE_ASYNC_H__
#include <vector>
#include "wifi_manager.h"

namespace sta_persistence_async
{
    void start_persist_task();
    void request_save(const std::vector<StaNetwork>&);
    void request_save(const std::vector<StaNetwork>&, const char* path);
    bool is_persisted(); // true si tout est écrit
}
#endif