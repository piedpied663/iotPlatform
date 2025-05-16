#pragma once
#ifndef __AP_PERSISTENCE_H__
#define __AP_PERSISTENCE_H__

#include <string>
#include "wifi_types.h"

namespace ap_persistence
{

    // Clés JSON
    static constexpr const char *KEY_SSID = "ssid";
    static constexpr const char *KEY_PASSWORD = "password";
    static constexpr const char *KEY_CHANNEL = "channel";

    // Sérialisation / désérialisation
    std::string to_json(const net_ap_config_t &cred);
    bool from_json(const std::string &json, net_ap_config_t &cred);

    // Persistance synchrone
    bool save(const net_ap_config_t &cred, const char *path);
    bool load(net_ap_config_t &cred, const char *path);

    // Versions par défaut
    bool save(const net_ap_config_t &cred);
    bool load(net_ap_config_t &cred);

} // namespace ap_persistence

#endif // __AP_PERSISTENCE_H__
