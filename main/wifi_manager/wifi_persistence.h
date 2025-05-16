#pragma once
#ifndef __WIFI_PERSISTENCE_H__
#define __WIFI_PERSISTENCE_H__
#include <vector>
#include <string>
#include "wifi_types.h"
#include "wifi_manager.h"
#include "wifi_persistence_async.h"

namespace wifi_persistence {
    bool save(const std::vector<StaNetwork>& nets);
    bool save(const std::vector<StaNetwork>& nets, const char* path);
    bool load(std::vector<StaNetwork>& nets);
    bool load(std::vector<StaNetwork>& nets, const char* path);
}
#endif