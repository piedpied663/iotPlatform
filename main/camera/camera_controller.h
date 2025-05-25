#pragma once
#ifndef __IOT_CAMERA_CONTROLLER_H__
#define __IOT_CAMERA_CONTROLLER_H__
#include "event_bus.h"
#include "esp_log.h"
#include "esp_camera.h"
#include <atomic>
#include "cJSON.h"
#include <string>
#include "camera.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace camera_controller
{
    struct camera_config_persist
    {
        uint8_t framesize;
        int8_t quality;
        int8_t contrast;
        int8_t brightness;
        int8_t saturation;
        int8_t sharpness;
        int8_t denoise;
        int8_t special_effect;
        int8_t wb_mode;
        int8_t ae_level;
        int16_t aec_value;
        int8_t agc_gain;
        int8_t gainceiling;

        bool awb;
        bool awb_gain;
        bool agc;
        bool bpc;
        bool wpc;
        bool raw_gma;
        bool lenc;
        bool hmirror;
        bool vflip;
        bool downsize;
        bool colorbar;
        bool aec;
        bool aec2;
    };

    class CameraController
    {
    private:
        static CameraController *_instance;

    public:
        static CameraController &getInstance()
        {
            if (!_instance)
                _instance = new CameraController();

            return *_instance;
        }

        esp_err_t initialize();

        camera_fb_t *tryCapture();

        void releaseCapture(camera_fb_t *frame)
        {
            if (frame)
                esp_camera_fb_return(frame);
        }

        bool isInitialized();
        sensor_t *getSensor();

    private:
        CameraController() = default;
        ~CameraController() = default;
        CameraController(const CameraController &) = delete;
        CameraController &operator=(const CameraController &) = delete;
        static constexpr const char *TAG = "CameraController";
        void apply_config(const camera_config_persist &cfg);
        void extract_config(camera_config_persist &cfg) const;
        static camera_config_t get_camera_config()
        {
            camera_config_t config{};

            // Commun à tous
            config.ledc_timer = LEDC_TIMER_0;
            config.ledc_channel = LEDC_CHANNEL_0;
            config.pixel_format = PIXFORMAT_JPEG;
            config.frame_size = FRAMESIZE_SVGA;
            config.jpeg_quality = 12;
            config.fb_count = 2;
            config.xclk_freq_hz = 20000000;

#if CONFIG_OV2640_SUPPORT
            // AI-Thinker ESP32-CAM
            config.pin_pwdn = 32;
            config.pin_reset = -1;
            config.pin_xclk = 0;
            config.pin_sccb_sda = 26;
            config.pin_sccb_scl = 27;
            config.pin_d7 = 35;
            config.pin_d6 = 34;
            config.pin_d5 = 39;
            config.pin_d4 = 36;
            config.pin_d3 = 21;
            config.pin_d2 = 19;
            config.pin_d1 = 18;
            config.pin_d0 = 5;
            config.pin_vsync = 25;
            config.pin_href = 23;
            config.pin_pclk = 22;

#elif CONFIG_OV7725_SUPPORT
            // Exemple générique OV7725
            config.pin_pwdn = -1;
            config.pin_reset = -1;
            config.pin_xclk = 21;
            config.pin_sccb_sda = 26;
            config.pin_sccb_scl = 27;
            config.pin_d7 = 35;
            config.pin_d6 = 34;
            config.pin_d5 = 39;
            config.pin_d4 = 36;
            config.pin_d3 = 19;
            config.pin_d2 = 18;
            config.pin_d1 = 5;
            config.pin_d0 = 4;
            config.pin_vsync = 25;
            config.pin_href = 23;
            config.pin_pclk = 22;

#elif CONFIG_OV3660_SUPPORT
            // À ajuster selon ton board
            config.pin_pwdn = -1;
            config.pin_reset = -1;
            config.pin_xclk = 32;
            config.pin_sccb_sda = 13;
            config.pin_sccb_scl = 12;
            config.pin_d7 = 19;
            config.pin_d6 = 36;
            config.pin_d5 = 39;
            config.pin_d4 = 35;
            config.pin_d3 = 34;
            config.pin_d2 = 14;
            config.pin_d1 = 5;
            config.pin_d0 = 4;
            config.pin_vsync = 27;
            config.pin_href = 25;
            config.pin_pclk = 22;

            // Répéter pour les autres modèles supportés :

#elif CONFIG_GC032A_SUPPORT
            // Pins hypothétiques, à adapter à ton schéma
            config.pin_pwdn = -1;
            config.pin_reset = -1;
            config.pin_xclk = 4;
            config.pin_sccb_sda = 18;
            config.pin_sccb_scl = 23;
            config.pin_d7 = 36;
            config.pin_d6 = 39;
            config.pin_d5 = 35;
            config.pin_d4 = 34;
            config.pin_d3 = 33;
            config.pin_d2 = 32;
            config.pin_d1 = 19;
            config.pin_d0 = 21;
            config.pin_vsync = 22;
            config.pin_href = 25;
            config.pin_pclk = 5;

#endif

            return config;
        }
        camera_config_persist m_config;

    public:
        /// Définit la taille de l’image (résolution)
        bool setFrameSize(framesize_t size);

        /// Définit la qualité JPEG (10 = haute qualité, 63 = faible)
        bool setQuality(int value);

        /// Définit le niveau de contraste [-2..2]
        bool setContrast(int level);

        /// Définit la luminosité [-2..2]
        bool setBrightness(int level);

        /// Définit la saturation des couleurs [-2..2]
        bool setSaturation(int level);

        /// Définit la netteté [-2..2]
        bool setSharpness(int level);

        /// Définit le niveau de débruitage (si disponible)
        bool setDenoise(int value); // uint8_t → valeur 0-255 (ou bool si hard limité)

        /// Applique un effet spécial (0 à 6)
        bool setSpecialEffect(int effect);

        /// Définit le mode de balance des blancs (0 à 4)
        bool setWbMode(int mode);

        /// Active ou désactive la balance des blancs automatique (AWB)
        bool setAwb(bool enable);

        /// Active ou désactive le gain AWB
        bool setAwbGain(bool enable);

        /// Active ou désactive l'auto-exposure control (AEC)
        bool setAec(bool enable);

        /// Active ou désactive le deuxième algorithme AEC (AEC2)
        bool setAec2(bool enable);

        /// Définit manuellement la valeur d’exposition (si AEC désactivé)
        bool setAecValue(int value); // 0 - 1200

        /// Corrige le niveau d’exposition automatique calculé
        bool setAeLevel(int level); // -2 à +2

        /// Active ou désactive l'auto-gain control (AGC)
        bool setAgc(bool enable);

        /// Définit manuellement le gain (si AGC désactivé)
        bool setAgcGain(int value); // 0 - 30

        /// Définit la limite haute du gain automatique
        bool setGainCeiling(int value); // 0 - 6

        /// Active/désactive la correction des pixels noirs
        bool setBpc(bool enable);

        /// Active/désactive la correction des pixels blancs
        bool setWpc(bool enable);

        /// Active/désactive la gamma correction sur RAW
        bool setRawGma(bool enable);

        /// Active/désactive la correction d’optique (Lens Correction)
        bool setLensCorrection(bool enable);

        /// Active ou désactive le mirroring horizontal
        bool setHorizontalMirror(bool enable);

        /// Active ou désactive le mirroring vertical
        bool setVerticalFlip(bool enable);

        /// Active/désactive le downsampling (réduction de charge CPU)
        bool setDownsizeEnable(bool enable); // lié à `dcw`

        /// Affiche une mire de test colorée à l’image
        bool setColorBar(bool enable);
        __attribute__((noinline)) bool load();
        __attribute__((noinline)) bool save(); // Sauvegarde directe l’état actuel

        // Demande asynchrone de sauvegarde d’un seul net_credential_t
        __attribute__((noinline)) void request_save();

        __attribute__((noinline)) bool from_json(const std::string &json);
        __attribute__((noinline)) std::string to_json();
        static void on_event(const Event &evt)
        {
            if (evt.type == EventType::FS_READY)
            {
                if (!CameraController::getInstance().isInitialized())
                    CameraController::getInstance().initialize();
            }
        }
        struct register_event_bus
        {
            register_event_bus()
            {
                EventBus::getInstance().subscribe(on_event, TAG);
            }
        };

        inline static register_event_bus reg_event_bus;
    };

} // namespace iot::camera
#endif // CONFIG_IOT_CAMERA