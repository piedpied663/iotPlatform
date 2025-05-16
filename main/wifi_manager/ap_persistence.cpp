#include "ap_persistence.h"
#include "esp_log.h"
#include "cJSON.h"
#include <cstdio>
#include <unistd.h>

namespace ap_persistence {
static const char* TAG = "ApPersist";

// Sérialise un seul réseau en JSON
std::string to_json(const net_ap_config_t& cred) {
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, KEY_SSID,     cred.ap_creds.ssid.c_str());
    cJSON_AddStringToObject(root, KEY_PASSWORD, cred.ap_creds.password.c_str());
    cJSON_AddNumberToObject(root, KEY_CHANNEL,  cred.ap_channel);

    char* buf = cJSON_PrintUnformatted(root);
    std::string out = buf ? buf : "";
    if (buf) cJSON_free(buf);
    cJSON_Delete(root);
    return out;
}

// Recrée un net_ap_config_t depuis une chaîne JSON
bool from_json(const std::string& json, net_ap_config_t& cred) {
    cJSON* root = cJSON_Parse(json.c_str());
    if (!root) {
        ESP_LOGE(TAG, "JSON invalide");
        return false;
    }
    cJSON* js = cJSON_GetObjectItemCaseSensitive(root, KEY_SSID);
    cJSON* jp = cJSON_GetObjectItemCaseSensitive(root, KEY_PASSWORD);
    cJSON* jc = cJSON_GetObjectItemCaseSensitive(root, KEY_CHANNEL);

    bool ok = false;
    if (cJSON_IsString(js) && cJSON_IsString(jp) && cJSON_IsNumber(jc)) {
        const std::string ssid = js->valuestring;
        if (!ssid.empty() && ssid.size() <= 32) {
            cred.ap_creds.ssid    = ssid;
            cred.ap_creds.password = jp->valuestring;
            cred.ap_channel       = jc->valueint;
            ok = true;
        } else {
            ESP_LOGW(TAG, "SSID invalide dans JSON: '%s'", ssid.c_str());
        }
    } else {
        ESP_LOGE(TAG, "Champs '%s' ou '%s' manquants ou non-strings", KEY_SSID, KEY_PASSWORD);
    }
    cJSON_Delete(root);
    return ok;
}

// Sauvegarde atomique
bool save(const net_ap_config_t& cred, const char* path) {
    std::string json = to_json(cred);
    if (json.empty()) {
        ESP_LOGE(TAG, "to_json a renvoyé vide");
        return false;
    }

    std::string tmp = std::string(path) + ".tmp";
    FILE* f = fopen(tmp.c_str(), "w");
    if (!f) {
        ESP_LOGE(TAG, "Impossible d’ouvrir %s", tmp.c_str());
        return false;
    }
    size_t len = json.size();
    size_t w   = fwrite(json.data(), 1, len, f);
    fflush(f);
    if (int fd = fileno(f); fd >= 0) fsync(fd);
    fclose(f);

    if (w != len) {
        ESP_LOGE(TAG, "Écriture incomplète (%zu/%zu)", w, len);
        return false;
    }
    if (rename(tmp.c_str(), path) != 0) {
        ESP_LOGE(TAG, "Rename %s → %s échoué", tmp.c_str(), path);
        return false;
    }
    ESP_LOGI(TAG, "AP config sauvegardée dans %s", path);
    return true;
}

// Chargement en mémoire puis parse
bool load(net_ap_config_t& cred, const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) {
        ESP_LOGE(TAG, "Impossible d’ouvrir %s", path);
        return false;
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);
    if (len <= 0) {
        fclose(f);
        ESP_LOGE(TAG, "Fichier vide ou taille invalide (%ld)", len);
        return false;
    }

    std::string json(len, '\0');
    size_t r = fread(&json[0], 1, len, f);
    fclose(f);
    if (r != size_t(len)) {
        ESP_LOGE(TAG, "Lecture incomplète (%zu/%ld)", r, len);
        return false;
    }

    if (!from_json(json, cred)) {
        ESP_LOGE(TAG, "from_json a échoué");
        return false;
    }
    ESP_LOGI(TAG, "AP config chargée: ssid='%s'", cred.ap_creds.ssid.c_str());
    return true;
}

// Versions par défaut
bool save(const net_ap_config_t& cfg) {
    return save(cfg, "/sdcard/ap_wifi.json");
}
bool load(net_ap_config_t& cfg) {
    return load(cfg, "/sdcard/ap_wifi.json");
}

}  // namespace ap_persistence
