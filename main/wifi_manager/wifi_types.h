#pragma once
#ifndef __WIFI_TYPES_H__
#define __WIFI_TYPES_H__

#include <string>

struct net_credential_t
{
    std::string ssid;
    std::string password;
};

struct net_ap_config_t
{
    net_credential_t ap_creds;
    uint8_t ap_channel;

    net_ap_config_t() : ap_channel(1) {}
    net_ap_config_t(net_credential_t ap_creds, uint8_t ap_channel = 1) : ap_creds(ap_creds), ap_channel(ap_channel) {}
    net_ap_config_t(const net_ap_config_t &other) : ap_creds(other.ap_creds), ap_channel(other.ap_channel) {}
};
#endif