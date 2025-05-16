#pragma once
#ifndef __sta_persistence_H__
#define __sta_persistence_H__
#include <vector>
#include <string>
#include "wifi_types.h"
#include "wifi_manager.h"
#include "sta_persistence_async.h"

namespace sta_persistence {
    bool save(const std::vector<StaNetwork>& nets);
    bool save(const std::vector<StaNetwork>& nets, const char* path);
    bool load(std::vector<StaNetwork>& nets);
    bool load(std::vector<StaNetwork>& nets, const char* path);
    std::string to_json(const std::vector<StaNetwork>& nets);
    bool from_json(const char* json, std::vector<StaNetwork>& nets);
}
#endif