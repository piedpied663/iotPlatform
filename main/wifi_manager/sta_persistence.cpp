#include "sta_persistence.h"
#include "esp_log.h"
#include "cJSON.h"
#include <fstream>
#include <sstream>
#include "esp_log.h"

namespace sta_persistence
{

    static const char *TAG = "WifiPersist";

    // 1. Clés JSON extraites
    static constexpr const char *JSON_KEY_STA = "sta_networks";
    static constexpr const char *JSON_KEY_SSID = "ssid";
    static constexpr const char *JSON_KEY_PASSWORD = "password";

    bool save(const std::vector<StaNetwork> &nets, const char *path)
    {
        // 1) Sérialisation via to_json()
        std::string json = to_json(nets);
        if (json.empty())
        {
            ESP_LOGE(TAG, "to_json returned empty string");
            return false;
        }

        // 2) Écriture atomique
        std::string tmp = std::string(path) + ".tmp";
        FILE *f = fopen(tmp.c_str(), "w");
        if (!f)
        {
            ESP_LOGE(TAG, "Ouverture échouée %s", tmp.c_str());
            return false;
        }
        size_t len = json.size();
        size_t w = fwrite(json.data(), 1, len, f);
        fflush(f);
        if (int fd = fileno(f); fd >= 0)
            fsync(fd);
        fclose(f);

        if (w != len)
        {
            ESP_LOGE(TAG, "Écriture incomplète (%zu/%zu)", w, len);
            return false;
        }
        if (rename(tmp.c_str(), path) != 0)
        {
            ESP_LOGE(TAG, "Rename %s → %s échoué", tmp.c_str(), path);
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

        // 1) Lecture intégrale du fichier
        FILE *f = fopen(path, "r");
        if (!f)
        {
            ESP_LOGE(TAG, "Ouverture échouée %s", path);
            return false;
        }
        fseek(f, 0, SEEK_END);
        long len = ftell(f);
        rewind(f);
        if (len <= 0)
        {
            fclose(f);
            ESP_LOGE(TAG, "Fichier vide ou taille invalide (%ld)", len);
            return false;
        }

        std::string json(len, '\0');
        size_t r = fread(&json[0], 1, len, f);
        fclose(f);
        if (r != size_t(len))
        {
            ESP_LOGE(TAG, "Lecture incomplète (%zu/%ld)", r, len);
            return false;
        }

        // 2) Désérialisation via from_json()
        if (!from_json(json.c_str(), nets))
        {
            ESP_LOGE(TAG, "from_json a échoué");
            return false;
        }

        // 3) Fallback si liste vide
        if (nets.empty())
        {
            ESP_LOGW(TAG, "Aucun réseau dans JSON, fallback défaut");
            wifi_manager::add_sta_network(CONFIG_IOT_WIFI_SSID, CONFIG_IOT_WIFI_PASSWD);
        }

        ESP_LOGI(TAG, "Chargé %zu réseaux depuis %s", nets.size(), path);
        return true;
    }

    std::string to_json(const std::vector<StaNetwork> &nets)
    {
        cJSON *root = cJSON_CreateObject();
        cJSON *arr = cJSON_AddArrayToObject(root, JSON_KEY_STA);

        for (const auto &net : nets)
        {
            if (net.ssid.empty() || net.ssid.length() > 32)
                continue;
            cJSON *obj = cJSON_CreateObject();
            cJSON_AddStringToObject(obj, JSON_KEY_SSID, net.ssid.c_str());
            cJSON_AddStringToObject(obj, JSON_KEY_PASSWORD, net.password.c_str());
            cJSON_AddItemToArray(arr, obj);
        }

        char *buf = cJSON_PrintUnformatted(root);
        std::string out = buf ? buf : "";
        if (buf)
            cJSON_free(buf);
        cJSON_Delete(root);
        return out;
    }

    bool from_json(const char *json, std::vector<StaNetwork> &nets)
    {
        cJSON *root = cJSON_Parse(json);
        if (!root)
        {
            ESP_LOGE(TAG, "from_json: parse error");
            return false;
        }
        cJSON *arr = cJSON_GetObjectItemCaseSensitive(root, JSON_KEY_STA);
        if (!cJSON_IsArray(arr))
        {
            ESP_LOGE(TAG, "from_json: '%s' not an array", JSON_KEY_STA);
            cJSON_Delete(root);
            return false;
        }

        nets.clear();
        cJSON *item = nullptr;
        cJSON_ArrayForEach(item, arr)
        {
            auto j_ssid = cJSON_GetObjectItemCaseSensitive(item, JSON_KEY_SSID);
            auto j_pwd = cJSON_GetObjectItemCaseSensitive(item, JSON_KEY_PASSWORD);
            if (cJSON_IsString(j_ssid) && cJSON_IsString(j_pwd))
            {
                const std::string ssid = j_ssid->valuestring;
                const std::string pwd = j_pwd->valuestring;
                if (!ssid.empty() && ssid.size() <= 32)
                    nets.emplace_back(StaNetwork{ssid, pwd});
            }
        }
        cJSON_Delete(root);
        return true;
    }
}
