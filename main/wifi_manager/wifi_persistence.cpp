#include "wifi_persistence.h"
#include "esp_log.h"
#include "cJSON.h"
#include <fstream>
#include <sstream>
#include "esp_log.h"

namespace wifi_persistence
{

    static const char *TAG = "WifiPersist";

    // 1. Clés JSON extraites
    static constexpr const char *JSON_KEY_STA = "sta_networks";
    static constexpr const char *JSON_KEY_SSID = "ssid";
    static constexpr const char *JSON_KEY_PASSWORD = "password";

    bool save(const std::vector<StaNetwork> &nets, const char *path)
    {
        // 2. Création de l’arbre JSON
        cJSON *root = cJSON_CreateObject();
        if (!root)
        {
            ESP_LOGE(TAG, "cJSON_CreateObject failed");
            return false;
        }
        cJSON *arr = cJSON_CreateArray();
        if (!arr)
        {
            ESP_LOGE(TAG, "cJSON_CreateArray failed");
            cJSON_Delete(root);
            return false;
        }
        cJSON_AddItemToObject(root, JSON_KEY_STA, arr);

        // 3. Remplissage du tableau
        for (const auto &net : nets)
        {
            if (net.ssid.empty() || net.ssid.length() > 32)
            {
                ESP_LOGW(TAG, "Ignoring invalid SSID: '%s'", net.ssid.c_str());
                continue;
            }
            cJSON *obj = cJSON_CreateObject();
            if (!obj)
            {
                ESP_LOGW(TAG, "cJSON_CreateObject for item failed");
                continue;
            }
            cJSON_AddStringToObject(obj, JSON_KEY_SSID, net.ssid.c_str());
            cJSON_AddStringToObject(obj, JSON_KEY_PASSWORD, net.password.c_str());
            cJSON_AddItemToArray(arr, obj);
        }

        // 4. Impression en mémoire (contiguë) pour éviter la fragmentation
        char *jsonStr = cJSON_PrintBuffered(root, 0, /*fmt=*/0);
        if (!jsonStr)
        {
            ESP_LOGE(TAG, "cJSON_PrintBuffered failed");
            cJSON_Delete(root);
            return false;
        }
        size_t len = strlen(jsonStr);

        // 5. Vérification rapide du JSON généré
        cJSON *verify = cJSON_Parse(jsonStr);
        if (!verify)
        {
            ESP_LOGE(TAG, "JSON verification failed");
            cJSON_free(jsonStr);
            cJSON_Delete(root);
            return false;
        }
        cJSON_Delete(verify);

        // 6. Écriture atomique avec fsync
        std::string tmpPath = std::string(path) + ".tmp";
        FILE *f = fopen(tmpPath.c_str(), "w");
        if (!f)
        {
            ESP_LOGE(TAG, "Failed to open %s for writing", tmpPath.c_str());
            cJSON_free(jsonStr);
            cJSON_Delete(root);
            return false;
        }
        size_t written = fwrite(jsonStr, 1, len, f);
        fflush(f);
        int fd = fileno(f);
        if (fd >= 0)
            fsync(fd);
        fclose(f);

        cJSON_free(jsonStr);
        cJSON_Delete(root);

        if (written != len)
        {
            ESP_LOGE(TAG, "Write error (%zu of %zu bytes)", written, len);
            return false;
        }

        if (rename(tmpPath.c_str(), path) != 0)
        {
            ESP_LOGE(TAG, "Failed to rename %s → %s", tmpPath.c_str(), path);
            return false;
        }

        ESP_LOGI(TAG, "Config Wi-Fi sauvegardée dans %s", path);
        return true;
    }

    bool load(std::vector<StaNetwork> &nets)
    {
        return load(nets, "/sdcard/wifi.txt");
    }

    bool save(std::vector<StaNetwork, std::allocator<StaNetwork>> const &nets)
    {
        return save(nets, "/sdcard/wifi.txt");
    }

    bool load(std::vector<StaNetwork> &nets, const char *path)
    {
        FILE *f = fopen(path, "r");
        if (!f)
        {
            ESP_LOGE(TAG, "Erreur: Impossible d’ouvrir %s (SD absente ?)", path);
            return false;
        }
        fseek(f, 0, SEEK_END);
        long len = ftell(f);
        rewind(f);
        if (len <= 0)
        {
            fclose(f);
            ESP_LOGE(TAG, "Erreur: Fichier vide ou taille invalide (%ld)", len);
            return false;
        }
        std::string json(len, '\0');
        size_t r = fread(&json[0], 1, len, f);
        fclose(f);
        if (r != len)
        {
            ESP_LOGE(TAG, "Erreur: fread a échoué");
            return false;
        }

        cJSON *root = cJSON_Parse(json.c_str());
        if (!root)
        {
            ESP_LOGE(TAG, "Erreur: cJSON_Parse a échoué (JSON corrompu ?)");
            return false;
        }
        cJSON *arr = cJSON_GetObjectItem(root, "sta_networks");
        if (!cJSON_IsArray(arr))
        {
            ESP_LOGE(TAG, "Erreur: champ 'sta_networks' absent ou invalide");
            cJSON_Delete(root);
            return false;
        }
        nets.clear();
        cJSON *obj = nullptr;
        cJSON_ArrayForEach(obj, arr)
        {
            cJSON *j_ssid = cJSON_GetObjectItem(obj, "ssid");
            cJSON *j_pwd = cJSON_GetObjectItem(obj, "password");
            if (cJSON_IsString(j_ssid) && cJSON_IsString(j_pwd))
            {
                std::string password = j_pwd->valuestring;
                std::string ssid = j_ssid->valuestring;
                if (ssid.length() > 32)
                    continue;
                nets.push_back({ssid, password});
            }
        }
        ESP_LOGI(TAG, "Config wifi chargée depuis SD (%s), %d réseaux", path, (int)nets.size());
        cJSON_Delete(root);
        return true;
    }
}
