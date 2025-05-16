#pragma once
#ifndef __AP_PERSISTENCE_ASYNC_H__
#define __AP_PERSISTENCE_ASYNC_H__

#include "wifi_types.h"

namespace ap_persistence_async
{

    // Demande asynchrone de sauvegarde d’un seul net_credential_t
    void request_save(const net_credential_t &cred,
                      const char *path = "/sdcard/ap_wifi.json");

} // namespace ap_persistence_async

#endif // __AP_PERSISTENCE_ASYNC_H__
