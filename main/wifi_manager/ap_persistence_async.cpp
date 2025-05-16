#include "ap_persistence_async.h"
#include "ap_persistence.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

namespace ap_persistence_async {
static const char* TAG = "ApPersistAsync";

struct SaveParams {
    net_credential_t cred;
    std::string      path;
};

static void save_task(void* arg) {
    auto* p = static_cast<SaveParams*>(arg);
    ESP_LOGI(TAG, "Démarrage tâche de sauvegarde AP");
    ap_persistence::save(p->cred, p->path.c_str());
    delete p;
    vTaskDelete(nullptr);
}

void request_save(const net_credential_t& cred,
                  const char* path /* = "/sdcard/ap_wifi.json" */) {
    auto* params = new SaveParams{cred, std::string(path)};
    xTaskCreate(save_task, "ap_save", 4096, params, 3, nullptr);
}

}  // namespace ap_persistence_async
