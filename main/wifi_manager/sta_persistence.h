#pragma once
#ifndef __sta_persistence_H__
#define __sta_persistence_H__
#include <vector>
#include <string>
#include "wifi_types.h"
#include "wifi_manager.h"
#include "sta_persistence_async.h"

namespace sta_persistence {
    bool save(const std::vector<net_credential_t>& nets);
    bool save(const std::vector<net_credential_t>& nets, const char* path);
    bool load(std::vector<net_credential_t>& nets);
    bool load(std::vector<net_credential_t>& nets, const char* path);
    std::string to_json(const std::vector<net_credential_t>& nets);
    bool from_json(const char* json, std::vector<net_credential_t>& nets);
}
#endif