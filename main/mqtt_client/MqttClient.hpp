#pragma once

#include "event_bus.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "mqtt_client.h"
#include <mutex>
#include <cstring>
#include <map>
#include <functional>
#include <string>
#include "cJSON.h"
#include "types.h"
#include "persistence.h"
#include "utils.h"
#include "macro.h"

#ifdef CONFIG_IOT_FEATURE_SD
#define MQTT_CFG_PATH "/sd/mqtt.bin"
#else
#define MQTT_CFG_PATH "/fs/mqtt.bin"
#endif

class MqttClient final
{
public:
    enum class MqttActuatorFilter
    {
        NONE = 0,
        SAME_IP = 0,
    };

    using PublisherCallback = std::function<std::string(const char *)>;
    using SubscribeCallback = std::function<std::string(const char *topic, const char *payload, int len)>;
    using MqttJsonHandler = std::function<void(const cJSON *root, cJSON *resp)>;

    static MqttClient &getInstance()
    {
        static MqttClient instance;
        return instance;
    }
    MqttClient(const MqttClient &) = delete;
    MqttClient &operator=(const MqttClient &) = delete;

    // Initialisation unique du client
    void init()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (is_initialized_)
            return;

        if (!mqtt_config.enabled)
        {
            ESP_LOGW(TAG, "MQTT is disabled");
            return;
        }

        esp_mqtt_client_config_t cfg = {};
        cfg.broker.address.uri = mqtt_config.broker_uri;
        cfg.credentials.username = mqtt_config.username;
        cfg.credentials.authentication.password = mqtt_config.password;
        cfg.credentials.client_id = utils::make_unique_client_id(mqtt_config.client_id).c_str();
        ESP_LOGI(TAG, "MQTT client id: %s \n uri : %s", cfg.credentials.client_id, mqtt_config.broker_uri);
        client_ = esp_mqtt_client_init(&cfg);
        esp_mqtt_client_register_event(client_, MQTT_EVENT_ANY, mqtt_event_handler, this);
        esp_mqtt_client_start(client_);

        publish_queue_ = xQueueCreate(10, sizeof(PublishMessage));
        xTaskCreate(publisher_task, "mqtt_publisher_task", 4096, this, 5, nullptr);
        xTaskCreate(auto_publisher_task, "mqtt_auto_pub_task", 4096, this, 5, nullptr);

        is_initialized_ = true;
    }

    // Publication asynchrone via queue
    bool publish(const char *topic, const char *payload, int qos = 1, bool retain = false)
    {
        PublishMessage msg{};
        strncpy(msg.topic, topic, sizeof(msg.topic) - 1);
        size_t len = strnlen(payload, sizeof(msg.payload) - 1);
        memcpy(msg.payload, payload, len);
        msg.payload[len] = '\0';
        msg.payload_len = len;
        msg.qos = qos;
        msg.retain = retain;

        if (xQueueSend(publish_queue_, &msg, pdMS_TO_TICKS(100)) != pdTRUE)
        {
            ESP_LOGE(TAG, "Queue publish full, message dropped");
            return false;
        }
        return true;
    }

    // Enregistrement d'un publisher périodique
    void registerPublisher(const char *topic, PublisherCallback cb, uint32_t interval_ms)
    {
        std::lock_guard<std::mutex> lock(pub_mutex_);
        autoPublishers_[std::string(topic)] = {cb, interval_ms, 0};
    }
    void registerSubscriber(const char *topic, const char *answer_topic, SubscribeCallback cb)
    {
        std::lock_guard lock(sub_mutex_);
        subscribers_[std::string(topic)] = SubscriberInfo{cb, std::string(answer_topic ? answer_topic : ""), false};
    }
    void registerMqttActuator(const char *topic_in, const char *topic_out, MqttJsonHandler handler, MqttActuatorFilter filter = MqttActuatorFilter::NONE)
    {
        registerSubscriber(topic_in, topic_out,
                           [handler, filter](const char *topic, const char *payload, size_t) -> std::string
                           {
                               ESP_LOGI(TAG, "Received topic: %s payload: %s", topic, payload);
                               std::string result;

                               cJSON *root = cJSON_Parse(payload);
                               if (!root || !cJSON_IsObject(root))
                               {
                                   ESP_LOGE(TAG, "Invalid JSON");
                                   if (root)
                                       cJSON_Delete(root);
                                   return result;
                               }

                               cJSON *resp = cJSON_Duplicate(root, true);
                               if (filter == MqttActuatorFilter::SAME_IP)
                               {
                                   cJSON *ip = cJSON_GetObjectItem(root, "ip");
                                   if (ip)
                                   {
                                       char *ip_str = cJSON_GetStringValue(ip);
                                       ESP_LOGI(TAG, "IP: %s, self IP: %s", ip_str, self_ip);
                                       if (strcmp(ip_str, self_ip) == 0)
                                       {
                                           handler(root, resp);
                                       }
                                   }
                               }
                               else if (filter == MqttActuatorFilter::NONE)
                               {
                                   handler(root, resp);
                               }

                               char *raw = cJSON_PrintUnformatted(resp);
                               if (raw)
                               {
                                   result = raw;
                                   cJSON_free(raw);
                               }

                               cJSON_Delete(root);
                               cJSON_Delete(resp);

                               ESP_LOGI(TAG, "Response: %s", result.c_str());
                               return result;
                           });
    }
    // Vérification de la connexion
    bool isConnected()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return is_connected_;
    }

private:
    static constexpr const char *broker_uri_key = "broker_uri";
    static constexpr const char *username_key = "username";
    static constexpr const char *password_key = "password";
    static constexpr const char *client_id_key = "client_id";
    static constexpr const char *enabled_key = "enabled";

    struct PublishMessage
    {
        char topic[128];
        char payload[256];
        size_t payload_len;
        int qos;
        bool retain;
    };

    struct AutoPublisher
    {
        PublisherCallback callback;
        uint32_t interval;
        uint32_t lastPub;
    };
    struct SubscriberInfo
    {
        SubscribeCallback callback;
        std::string answer_topic;
        bool subscribed;
    };

    esp_mqtt_client_handle_t client_;
    QueueHandle_t publish_queue_;
    bool is_initialized_;
    bool is_connected_;

    std::mutex mutex_;
    std::mutex pub_mutex_;
    std::mutex sub_mutex_;

    std::map<std::string, AutoPublisher> autoPublishers_;
    std::map<std::string, SubscriberInfo> subscribers_;

    static constexpr const char *TAG = "[MQTT]";
    inline static char self_ip[16];
    inline static char self_host[32];
    MqttClient()
        : client_(nullptr), publish_queue_(nullptr), is_initialized_(false), is_connected_(false) {}

    ~MqttClient()
    {
        if (client_)
        {
            esp_mqtt_client_stop(client_);
            esp_mqtt_client_destroy(client_);
        }
        if (publish_queue_)
            vQueueDelete(publish_queue_);
    }

    void listenSubscribers()
    {
        std::lock_guard lock(sub_mutex_);
        if (!client_ || !is_connected_)
            return;

        for (auto &[topic, sub] : subscribers_)
        {
            if (!sub.subscribed)
            {
                int msg_id = esp_mqtt_client_subscribe(client_, topic.c_str(), 1);
                sub.subscribed = (msg_id >= 0);
                ESP_LOGI(TAG, "Subscribed to %s : %s", topic.c_str(), sub.subscribed ? "success" : "fail");
            }
        }
    }

    // Handler d'événements MQTT
    static void mqtt_event_handler(void *arg, esp_event_base_t, int32_t event_id, void *event_data)
    {
        auto *instance = static_cast<MqttClient *>(arg);
        std::lock_guard<std::mutex> lock(instance->mutex_);

        switch (event_id)
        {
        case MQTT_EVENT_CONNECTED:
        {
            ESP_LOGI(TAG, "MQTT connected");
            instance->is_connected_ = true;
            instance->listenSubscribers();
            utils::emitEvent(EventType::MQTT_CONNECTED);
            break;
        }

        case MQTT_EVENT_DISCONNECTED:
        {
            ESP_LOGW(TAG, "MQTT disconnected");
            instance->is_connected_ = false;
            utils::emitEvent(EventType::MQTT_DISCONNECTED);
            break;
        }

        case MQTT_EVENT_DATA:
        {
            auto *event = static_cast<esp_mqtt_event_handle_t>(event_data);
            std::string topic(event->topic, event->topic_len);
            std::string payload(event->data, event->data_len);

            // Emit as EventBus event (universel)
            Event evt = {};
            evt.type = EventType::MQTT_MESSAGE;

            // Embeds as JSON in data
            snprintf((char *)evt.data, sizeof(evt.data),
                     "{\"topic\":\"%s\",\"payload\":\"%s\"}", topic.c_str(), payload.c_str());
            evt.data_len = strlen((char *)evt.data);

            EventBus::getInstance().emit(evt);

            // Continue to local callbacks if needed (compat)
            for (const auto &[sub_topic, sub] : instance->subscribers_)
            {
                if (sub_topic == topic)
                {
                    std::string res = sub.callback(topic.c_str(), payload.c_str(), topic.length());
                    if (!res.empty())
                        esp_mqtt_client_publish(instance->client_, sub.answer_topic.c_str(), res.c_str(), res.length(), 1, 0);
                }
            }
            break;
        }

        case MQTT_EVENT_ERROR:
        {
            ESP_LOGE(TAG, "MQTT error");
            utils::emitEvent(EventType::MQTT_ERROR);
            break;
        }

        default:
            break;
        }
    }

    // Tâche de publication asynchrone
    static void publisher_task(void *arg)
    {
        auto &instance = *reinterpret_cast<MqttClient *>(arg);
        PublishMessage msg;

        while (true)
        {
            if (xQueueReceive(instance.publish_queue_, &msg, portMAX_DELAY))
            {
                if (instance.isConnected())
                {
                    int msg_id = esp_mqtt_client_publish(
                        instance.client_, msg.topic, msg.payload, msg.payload_len, msg.qos, msg.retain);
                    if (msg_id == -1)
                    {
                        ESP_LOGE(TAG, "Publish failed");
                    }
                    // else
                    // {
                    //     ESP_LOGI(TAG, "Published msg_id=%d", msg_id);
                    // }
                }
                else
                {
                    ESP_LOGW(TAG, "MQTT disconnected, dropping message");
                }
            }
        }
    }

    // Tâche de publication périodique intégrée
    static void auto_publisher_task(void *arg)
    {
        auto &instance = *reinterpret_cast<MqttClient *>(arg);
        uint32_t tick_ms;

        while (true)
        {
            vTaskDelay(pdMS_TO_TICKS(500));
            tick_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;

            std::lock_guard<std::mutex> lock(instance.pub_mutex_);
            for (auto &entry : instance.autoPublishers_)
            {
                const auto &topic = entry.first;
                auto &pub = entry.second;

                if ((tick_ms - pub.lastPub) >= pub.interval)
                {
                    std::string payload = pub.callback(topic.c_str());
                    instance.publish(topic.c_str(), payload.c_str(), 1, false);
                    pub.lastPub = tick_ms;
                }
            }
        }
    }

    // // Emission d'événements via EventBus
    // void emitEvent(EventType type)
    // {
    //     Event e{};
    //     e.type = type;
    //     EventBus::getInstance().emit(e);
    // }

    static bool load()
    {
        return persist::load_struct(MQTT_CFG_PATH, &mqtt_config, sizeof(mqtt_config));
    }
    static void task_save_mqtt(void)
    {
        xTaskCreate([](void *)
                    { save(); vTaskDelete(nullptr); }, "save_mqtt", 4096, nullptr, tskIDLE_PRIORITY + 1, nullptr);
    }
    static bool save()
    {
        return persist::save_struct(MQTT_CFG_PATH, &mqtt_config, sizeof(mqtt_config));
    }
    static bool mqtt_to_json(char *out_buf, size_t buf_len)
    {
        cJSON *root = cJSON_CreateObject();
        if (!root)
            return false;

        cJSON_AddStringToObject(root, broker_uri_key, mqtt_config.broker_uri);
        cJSON_AddStringToObject(root, username_key, mqtt_config.username);
        cJSON_AddStringToObject(root, password_key, mqtt_config.password);
        cJSON_AddStringToObject(root, client_id_key, mqtt_config.client_id);
        cJSON_AddBoolToObject(root, enabled_key, mqtt_config.enabled);
        char *json_string = cJSON_Print(root);
        cJSON_Delete(root);

        if (!json_string)
            return false;

        size_t json_len = strlen(json_string);
        bool success = false;

        if (json_len < buf_len)
        {
            strcpy(out_buf, json_string);
            success = true;
        }

        cJSON_free(json_string);
        return success;
    }

    static bool mqtt_from_json(const char *json, mqtt_client_config_t &config)
    {
        cJSON *root = cJSON_Parse(json);
        if (!root)
            return false;
        cJSON *broker_uri = cJSON_GetObjectItem(root, broker_uri_key);
        cJSON *username = cJSON_GetObjectItem(root, username_key);
        cJSON *password = cJSON_GetObjectItem(root, password_key);
        cJSON *client_id = cJSON_GetObjectItem(root, client_id_key);
        cJSON *enabled = cJSON_GetObjectItem(root, enabled_key);
        if (!cJSON_IsString(broker_uri) || !cJSON_IsString(username) ||
            !cJSON_IsString(password) || !cJSON_IsString(client_id) ||
            !cJSON_IsBool(enabled))
        {
            cJSON_Delete(root);
            return false;
        }

        strncpy(config.broker_uri, broker_uri->valuestring, MAX_BROKER_URI_LEN - 1);
        config.broker_uri[MAX_BROKER_URI_LEN - 1] = '\0';

        strncpy(config.username, username->valuestring, MAX_USERNAME_LEN - 1);
        config.username[MAX_USERNAME_LEN - 1] = '\0';

        strncpy(config.password, password->valuestring, MAX_PASSWORD_LEN - 1);
        config.password[MAX_PASSWORD_LEN - 1] = '\0';

        strncpy(config.client_id, client_id->valuestring, MAX_CLIENT_ID_LEN - 1);
        config.client_id[MAX_CLIENT_ID_LEN - 1] = '\0';

        config.enabled = cJSON_IsTrue(enabled);

        cJSON_Delete(root);
        return true;
    }

    static bool mqtt_status_to_json(char *out_buf, size_t buf_len)
    {
        cJSON *root = cJSON_CreateObject();
        if (!root)
            return false;

        // parcourir autoPublishers_

        cJSON_AddBoolToObject(root, enabled_key, mqtt_config.enabled);
        // 3) parcourir les publishers
        cJSON *arr = cJSON_CreateArray();
        if (!arr)
        {
            cJSON_Delete(root);
            return false;
        }
        cJSON_AddItemToObject(root, "publishers", arr);

        // si votre map est protégée par un mutex :
        // ============================
        // PUBLISHERS
        // ============================
        {
            auto &instance = MqttClient::getInstance();

            std::lock_guard<std::mutex> lock(instance.pub_mutex_);
            for (auto const &[topic, pub] : instance.autoPublishers_)
            {
                // appeler le callback pour récupérer la valeur actuelle
                std::string payload = pub.callback(topic.c_str());

                // créer l’objet JSON pour ce publisher
                cJSON *obj = cJSON_CreateObject();
                if (!obj)
                    continue; // en cas d’erreur on skip

                cJSON_AddStringToObject(obj, "topic", topic.c_str());
                cJSON_AddStringToObject(obj, "payload", payload.c_str());
                cJSON_AddNumberToObject(obj, "interval_ms", pub.interval);
                cJSON_AddNumberToObject(obj, "lastPub", pub.lastPub);

                // ajouter dans le tableau
                cJSON_AddItemToArray(arr, obj);
            }
        }
        // ============================
        // SUBSCRIBERS
        // ============================
        {
            auto &instance = MqttClient::getInstance();
            cJSON *arr = cJSON_CreateArray();
            if (!arr)
            {
                cJSON_Delete(root);
                return false;
            }
            cJSON_AddItemToObject(root, "subscribers", arr);

            std::lock_guard<std::mutex> lock(instance.sub_mutex_);
            for (auto const &[topic, sub] : instance.subscribers_)
            {
                cJSON *obj = cJSON_CreateObject();
                if (!obj)
                    continue;

                cJSON_AddStringToObject(obj, "topic", topic.c_str());
                cJSON_AddStringToObject(obj, "answer_topic", sub.answer_topic.c_str());

                cJSON_AddBoolToObject(obj, "subscribed", sub.subscribed);

                // Tu peux ajouter ici d'autres métadonnées si présentes dans SubscriberInfo,
                // comme un timestamp, compteur, etc. Exemples :
                // cJSON_AddNumberToObject(obj, "message_count", sub.counter);
                // cJSON_AddStringToObject(obj, "source", sub.source_id);

                cJSON_AddItemToArray(arr, obj);
            }
        }
        // 4) transformer en string
        char *json_string = cJSON_PrintUnformatted(root);
        cJSON_Delete(root);
        if (!json_string)
            return false;

        // 5) copier dans out_buf si ça rentre
        bool success = false;
        size_t len = strlen(json_string);
        if (len < buf_len)
        {
            memcpy(out_buf, json_string, len + 1);
            success = true;
        }
        cJSON_free(json_string);
        return success;
    }

    inline static bool flag_iot_hosts_registred = false;

    static void on_event(const Event *evt)
    {
        if (evt->type == EventType::FS_READY)
        {
            if (!load())
            {
                ESP_LOGW(TAG, "Using default MQTT config");
            }
            else
            {
                ESP_LOGI(TAG, "MQTT config loaded");
            }
        }
        else if (evt->type == EventType::WIFI_STA_CONNECTED)
        {
            if (evt->data_len >= sizeof(iot_mqtt_status_t))
            {
                const iot_mqtt_status_t *status = reinterpret_cast<const iot_mqtt_status_t *>(evt->data);
                strncpy(self_ip, status->ip, sizeof(self_ip) - 1);
                strncpy(self_host, status->host, sizeof(self_host) - 1);
                self_ip[sizeof(self_ip) - 1] = '\0';
                self_host[sizeof(self_host) - 1] = '\0';
            }
            if (!MqttClient::getInstance().is_initialized_)
            {

                MqttClient::getInstance().init();

                /// now use
                ////struct iot_mqtt_status_t
                // cJSON *root = cJSON_Parse((const char *)evt.data);
                // if (root)
                // {
                //     cJSON *ip = cJSON_GetObjectItem(root, "ip");
                //     cJSON *host = cJSON_GetObjectItem(root, "host");

                //     if (ip && cJSON_IsString(ip))
                //         strncpy(self_ip, ip->valuestring, sizeof(self_ip) - 1);

                //     if (host && cJSON_IsString(host))
                //         strncpy(self_host, host->valuestring, sizeof(self_host) - 1);

                //     cJSON_Delete(root);
                // }
            }
        }
        else if (evt->type == EventType::MQTT_CONNECTED)
        {
            if (!flag_iot_hosts_registred)
            {

                MqttClient::getInstance().registerPublisher("iot/hosts", [&](const char *)
                                                            {
                                                    int ip1 = 0, ip2 = 0, ip3 = 0, ip4 = 0;
                                                    sscanf(self_ip, "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4);
                                                    int heap = esp_get_free_heap_size();
                                                    cJSON *root = cJSON_CreateObject();
                                                    cJSON_AddNumberToObject(root, "ip_0", ip1);
                                                    cJSON_AddNumberToObject(root, "ip_1", ip2);
                                                    cJSON_AddNumberToObject(root, "ip_2", ip3);
                                                    cJSON_AddNumberToObject(root, "ip_3", ip4);
                                                    cJSON_AddStringToObject(root, "host", self_host[0] ? self_host : CONFIG_IOT_HOSTNAME);
                                                    cJSON_AddNumberToObject(root, "heap_free", heap);
                                                    for (auto &[name, id] : features::get_named_features()) {
                                                        cJSON_AddNumberToObject(root, name, id);
                                                    }
                                                    char *json = cJSON_PrintUnformatted(root);
                                                    std::string result = json ? json : "";
                                                    if (json) cJSON_free(json);
                                                    cJSON_Delete(root);
                                                    return result; }, 5000);

                flag_iot_hosts_registred = true;
                ESP_LOGI(TAG, "IOT hosts registered");
            }
        }
        else if (evt->type == EventType::MQTT_CONFIG_REQUEST_JSON)
        {
            if (evt->user_ctx)
            {
                Event resp_evt = {};
                resp_evt.type = EventType::MQTT_CONFIG_ANSWER_JSON;
                char json[512];
                mqtt_to_json(json, sizeof(json));
                BIND_DATA_EVENT_JSON(resp_evt, json);
                // Envoie la réponse
                xQueueSend((QueueHandle_t)evt->user_ctx, &resp_evt, 0);
            }
        }
        else if (evt->type == EventType::MQTT_POST_REQUEST)
        {
            if (evt->user_ctx)
            {
                // 1) parser le JSON (evt.data, evt.data_len)
                const char *json = reinterpret_cast<const char *>(evt->user_ctx);
                // ex : sta_from_json(json);

                // 2) appliquer la config Wi-Fi, écrire en flash…

                if (mqtt_from_json(json, mqtt_config))
                {
                    task_save_mqtt();
                }
                // 3) envoyer l’ack
                Event resp_evt{};
                BIND_DATA_EVENT_JSON(resp_evt, json);
                resp_evt.type = EventType::MQTT_POST_ANSWER;
                xQueueSend((QueueHandle_t)evt->user_ctx, &resp_evt, 0);
            }
        }
        else if (evt->type == EventType::MQTT_STATUS_REQUEST_JSON)
        {
            if (evt->user_ctx)
            {
                Event resp_evt = {};
                char json[512];
                mqtt_status_to_json(json, sizeof(json));
                BIND_DATA_EVENT_JSON(resp_evt, json);
                resp_evt.type = EventType::MQTT_STATUS_ANSWER_JSON; // Envoie la réponse
                xQueueSend((QueueHandle_t)evt->user_ctx, &resp_evt, 0);
            }
        }
    }

    struct register_event_bus
    {
        register_event_bus()
        {
            EventBus::getInstance().subscribe(on_event, TAG);
        }
    };

    inline static register_event_bus register_event_bus_;

    inline static mqtt_client_config_t mqtt_config = {
        .broker_uri = CONFIG_IOT_MQTT_BROKER_URI,
        .username = CONFIG_IOT_MQTT_BROKER_USERNAME,
        .password = CONFIG_IOT_MQTT_BROKER_PASSWORD,
        .client_id = CONFIG_IOT_MQTT_BROKER_CLIENT_ID,
        .enabled = true};
};
