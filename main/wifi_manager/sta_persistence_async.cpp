#include "sta_persistence_async.h"
#include "sta_persistence.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
namespace sta_persistence_async {
static const char* TAG = "WifiPersistAsync";

struct SaveParams {
    std::vector<net_credential_t> nets;
    std::string            path;
};

void save_task(void* arg) {
    auto* p = static_cast<SaveParams*>(arg);
    ESP_LOGI(TAG, "Démarrage tâche de sauvegarde STA");
    sta_persistence::save(p->nets, p->path.c_str());
    delete p;
    vTaskDelete(nullptr);
}

void request_save(const std::vector<net_credential_t>& nets,
                  const char* path /* = "/sdcard/wifi.txt" */) {
    auto* params = new SaveParams{nets, std::string(path)};
    // stack 4096, priorité 3, tâche auto-supprimée à la fin
    xTaskCreate(save_task, "sta_save", 4096, params, 3, nullptr);
}

}  // namespace wifi_persistence_async
