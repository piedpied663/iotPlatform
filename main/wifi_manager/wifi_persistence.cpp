#include "wifi_persistence.h"
#include "esp_log.h"
#include "cJSON.h"
#include <fstream>
#include <sstream>
#include "esp_log.h"

namespace wifi_persistence
{
    static const char *TAG = "WifiPersist";

    bool save(const std::vector<StaNetwork> &nets, const char *path)
    {
        cJSON *root = cJSON_CreateObject();
        cJSON *arr = cJSON_CreateArray();
        for (const auto &net : nets)
        {
            // Anti-corruption : SSID obligatoire, max 32 !
            if (net.ssid.empty() || net.ssid.length() > 32)
                continue;
            cJSON *obj = cJSON_CreateObject();
            cJSON_AddStringToObject(obj, "ssid", net.ssid.c_str());
            cJSON_AddStringToObject(obj, "password", net.password.c_str());
            cJSON_AddItemToArray(arr, obj);
        }
        cJSON_AddItemToObject(root, "sta_networks", arr);

        char *jsonStr = cJSON_PrintUnformatted(root);
        if (!jsonStr)
        {
            cJSON_Delete(root);
            ESP_LOGE(TAG, "Erreur: cJSON_PrintUnformatted a échoué");
            return false;
        }

        FILE *f = fopen(path, "w");
        if (!f)
        {
            ESP_LOGE(TAG, "Erreur: Impossible d’ouvrir %s (SD absente ?)", path);
            cJSON_free(jsonStr);
            cJSON_Delete(root);
            return false;
        }
        size_t w = fwrite(jsonStr, 1, strlen(jsonStr), f);
        fflush(f); 
        fclose(f);
        cJSON_free(jsonStr);
        cJSON_Delete(root);
        if (w == 0)
        {
            ESP_LOGE(TAG, "Erreur: fwrite a échoué");
            return false;
        }
        ESP_LOGI(TAG, "Config wifi sauvegardée sur SD (%s)", path);
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
                nets.push_back({j_ssid->valuestring, j_pwd->valuestring});
            }
        }
        ESP_LOGI(TAG, "Config wifi chargée depuis SD (%s), %d réseaux", path, (int)nets.size());
        cJSON_Delete(root);
        return true;
    }
}
