#pragma once
#ifndef __WIFI_TYPES_H__
#define __WIFI_TYPES_H__

#include <string>

struct StaNetwork
{
    std::string ssid;
    std::string password;
};
#endif