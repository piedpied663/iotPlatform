#pragma once

#include <vector>

// #define CONFIG_IOT_FEATURE_CAMERA 1
// #define CONFIG_IOT_FEATURE_SD 1
// #define CONFIG_IOT_FEATURE_FLASHLED 1
// Identifiants fixes et stables
namespace features
{

#define FEATURE_ID_CAMERA 1
#define FEATURE_ID_FLASHLED 2
#define FEATURE_ID_SDCARD 3
#define FEATURE_ID_RELAY 4
#define FEATURE_ID_SECURE 5
#define FEATURE_ID_BRIDGE 6

    inline std::vector<std::pair<const char *, int>> get_named_features()
    {
        std::vector<std::pair<const char *, int>> features;

#ifdef CONFIG_IOT_FEATURE_CAMERA
        features.emplace_back("camera", FEATURE_ID_CAMERA);
#endif

#ifdef CONFIG_IOT_FEATURE_FLASHLED
        features.emplace_back("flashled", FEATURE_ID_FLASHLED);
#endif

#ifdef CONFIG_IOT_FEATURE_SD
        features.emplace_back("sd", FEATURE_ID_SDCARD);
#endif

#ifdef CONFIG_IOT_FEATURE_RELAY
        features.emplace_back("relay", FEATURE_ID_RELAY);
#endif

#ifdef CONFIG_IOT_FEATURE_SECURE
        features.emplace_back("secure", FEATURE_ID_SECURE);
#endif

#ifdef CONFIG_IOT_FEATURE_BRIDGE
        features.emplace_back("bridge", FEATURE_ID_BRIDGE);
#endif

        return features;
    }

}