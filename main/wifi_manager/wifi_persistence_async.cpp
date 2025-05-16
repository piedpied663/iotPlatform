#include "wifi_persistence_async.h"
#include "wifi_persistence.h"
#include "esp_log.h"
#include "freertos/queue.h"
#include "freertos/task.h"

namespace wifi_persistence_async
{

    static QueueHandle_t persist_queue = nullptr;
    static bool dirty = false;
    static bool persisted = true;
    static constexpr const char *TAG = "WifiPersistAsync";
    static void persist_task(void *)
    {
        std::vector<StaNetwork> nets;
        while (1)
        {
            // Attend une demande de sauvegarde (pointeur sur la liste en RAM)
            if (xQueueReceive(persist_queue, &nets, portMAX_DELAY) == pdTRUE)
            {
                dirty = true;
                persisted = false;
                // Ecrit sur SD (synchrone, mais hors contexte REST !)
                wifi_persistence::save(nets);
                dirty = false;
                persisted = true;
                ESP_LOGI(TAG, "Persistance SD async effectuée !");
            }
        }
    }

    void start_persist_task()
    {
        if (!persist_queue)
            persist_queue = xQueueCreate(1, sizeof(std::vector<StaNetwork>));
        xTaskCreate(persist_task, "persist_task", 4096, nullptr, 5, nullptr);
    }

    void request_save(const std::vector<StaNetwork> &nets)
    {
        request_save(nets, "/sdcard/wifi.txt");
    }

    void request_save(const std::vector<StaNetwork> &nets, const char * /*path*/)
    {
        if (persist_queue)
        {
            std::vector<StaNetwork> copy = nets; // Copie car la RAM peut changer
            xQueueOverwrite(persist_queue, &copy);
        }
    }

    bool is_persisted()
    {
        return persisted;
    }
}
