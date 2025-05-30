#pragma once

#include "esp_err.h"

namespace ota_manager
{
    /// @brief Lance la mise à jour OTA depuis l'URL donnée
    /// @param url URL complète du firmware binaire
    void start_update(const char* url);

    /// @brief Doit être appelé au démarrage du firmware
    /// Permet de valider l'image si le boot a réussi
    void validate_image();
}
