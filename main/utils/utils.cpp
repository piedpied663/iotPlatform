#include "utils.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <string>

namespace utils
{

    // Renvoie une chaîne composée de 'base_id' + 4 chiffres aléatoires
    std::string make_unique_client_id(const char *base_id)
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dist(0, 9999);

        int rnd = dist(gen);
        std::ostringstream oss;
        oss << base_id
            << std::setw(4)      // largeur 4
            << std::setfill('0') // remplissage avec '0'
            << rnd;
        return oss.str();
    }

    uint32_t hash_struct(const void *data, size_t len)
    {
        // Simple XOR hash, pas cryptographique mais rapide pour du cache
        const uint8_t *d = reinterpret_cast<const uint8_t *>(data);
        uint32_t h = 0;
        for (size_t i = 0; i < len; ++i)
            h = (h * 33) ^ d[i];
        return h;
    }

    void emitEventData(EventType type, const void *data, size_t data_len)
    {
        Event e{};
        e.type = type;
        memcpy(e.data, data, data_len);
        e.data_len = data_len;
        EventBus::getInstance().emit(e);
    }

    void emitEvent(EventType type, void *user_ctx)
    {
        Event e{};
        e.type = type;
        if (user_ctx != nullptr)
        {
            e.user_ctx = user_ctx;
            e.data_len = sizeof(user_ctx);
        }

        EventBus::getInstance().emit(e);
    }

    std::string mac_to_string(const uint8_t *mac)
    {
        char buf[18];
        snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        return std::string(buf);
    }

}