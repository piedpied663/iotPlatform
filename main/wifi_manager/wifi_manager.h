#pragma once
#include "esp_err.h"
#include <vector>
#include <string>
#include "wifi_api.h"
#include "esp_netif_types.h"
#include "wifi_types.h"

namespace wifi_manager
{

    enum class Status
    {
        INIT,
        AP_STARTED,
        STA_CONNECTING,
        STA_CONNECTED,
        STA_DISCONNECTED,
        ALL_STA_FAILED,
        ERROR
    };

    typedef void (*StatusCallback)(Status);

    void wifi_reconnect_async(void *);

    // Ajoute un réseau STA à la liste (avant start)
    bool add_sta_network(const std::string &ssid, const std::string &password);
    void remove_sta_network(const std::string &ssid);
    const std::vector<StaNetwork> &get_sta_list();
    std::vector<StaNetwork> &get_sta_list_mutable();

    // Efface tous les réseaux STA
    void clear_sta_networks();
    void cleanup_netifs();

    // Lance tout le wifi (APSTA, async)
    void start_apsta_async(StatusCallback cb = nullptr);

    // Obtenir l’état courant
    Status get_status();

    // Infos IP
    const char *get_ap_ip();
    const char *get_sta_ip();


}
