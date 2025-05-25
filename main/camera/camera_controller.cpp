#include "camera_controller.h"
#include "cJSON.h"
#include "persistence.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef CONFIG_IOT_FEATURE_SD
#define CAMERA_CFG_PATH "/sd/camera.bin"
#else
#define CAMERA_CFG_PATH "/fs/camera.bin"
#endif

namespace camera_controller
{
    // Constantes clés JSON
    static constexpr const char *TAG = "[CameraController]";
    static constexpr const char *KEY_FRAME_SIZE = "frame_size";
    static constexpr const char *KEY_JPEG_QUALITY = "jpeg_quality";
    static constexpr const char *KEY_CONTRAST = "contrast";
    static constexpr const char *KEY_BRIGHTNESS = "brightness";
    static constexpr const char *KEY_SATURATION = "saturation";
    static constexpr const char *KEY_AEC_VALUE = "aec_value";
    static constexpr const char *KEY_AE_LEVEL = "ae_level";
    static constexpr const char *KEY_AWB = "awb";
    static constexpr const char *KEY_AWB_GAIN = "awb_gain";
    static constexpr const char *KEY_AGC = "agc";
    static constexpr const char *KEY_AGC_GAIN = "agc_gain";
    static constexpr const char *KEY_BPC = "bpc";
    static constexpr const char *KEY_WPC = "wpc";
    static constexpr const char *KEY_RAW_GMA = "raw_gma";
    static constexpr const char *KEY_LENC = "lenc";
    static constexpr const char *KEY_HMIRROR = "hmirror";
    static constexpr const char *KEY_VFLIP = "vflip";
    static constexpr const char *KEY_DOWNSIZE = "downsize";
    static constexpr const char *KEY_COLORBAR = "colorbar";
    static constexpr const char *KEY_SHARPNESS = "sharpness";
    static constexpr const char *KEY_DENOISE = "denoise";
    static constexpr const char *KEY_SPECIAL_EFFECT = "special_effect";
    static constexpr const char *KEY_WB_MODE = "wb_mode";
    static constexpr const char *KEY_AEC = "aec";
    static constexpr const char *KEY_AEC2 = "aec2";
    static constexpr const char *KEY_GAINCEILING = "gainceiling";

    CameraController *CameraController::_instance = nullptr;
    bool initialized = false;
    sensor_t *sensor = nullptr;
    esp_err_t CameraController::initialize()
    {
        if (initialized)
            return ESP_OK;

        ESP_LOGI(TAG, "Initializing camera...");
        const camera_config_t &config = get_camera_config();
        esp_err_t err = esp_camera_init(&config);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Camera init failed: %s", esp_err_to_name(err));
            return ESP_FAIL;
        }
        ESP_LOGI(TAG, "Camera initialized");
        initialized = true;
        ESP_LOGI(TAG, "Getting camera sensor...");
        sensor = esp_camera_sensor_get();
        if (!sensor)
        {
            ESP_LOGE(TAG, "Failed to get sensor");
            initialized = false;
            return ESP_FAIL;
        }

        load();
        Event e{};
        e.type = EventType::CAMERA_INIT_DONE;
        EventBus::getInstance().emit(e);
        return ESP_OK;
    }
    camera_fb_t *CameraController::tryCapture()
    {
        if (!initialized)
            return nullptr;

        return esp_camera_fb_get();
    }

    bool CameraController::isInitialized()
    {
        return initialized;
    }

    sensor_t *CameraController::getSensor()
    {
        return sensor;
    }
    void CameraController::apply_config(const camera_config_persist &cfg)
    {
        setFrameSize(static_cast<framesize_t>(cfg.framesize));
        setQuality(cfg.quality);
        setContrast(cfg.contrast);
        setBrightness(cfg.brightness);
        setSaturation(cfg.saturation);
        setSharpness(cfg.sharpness);
        setDenoise(cfg.denoise);
        setSpecialEffect(cfg.special_effect);
        setWbMode(cfg.wb_mode);
        setAeLevel(cfg.ae_level);
        setAecValue(cfg.aec_value);
        setAgcGain(cfg.agc_gain);
        setGainCeiling(cfg.gainceiling);

        setAwb(cfg.awb);
        setAwbGain(cfg.awb_gain);
        setAgc(cfg.agc);
        setBpc(cfg.bpc);
        setWpc(cfg.wpc);
        setRawGma(cfg.raw_gma);
        setLensCorrection(cfg.lenc);
        setHorizontalMirror(cfg.hmirror);
        setVerticalFlip(cfg.vflip);
        setDownsizeEnable(cfg.downsize);
        setColorBar(cfg.colorbar);
        setAec(cfg.aec);
        setAec2(cfg.aec2);
    }
    void CameraController::extract_config(camera_config_persist &cfg) const
    {
        cfg.framesize = sensor->status.framesize;
        cfg.quality = sensor->status.quality;
        cfg.contrast = sensor->status.contrast;
        cfg.brightness = sensor->status.brightness;
        cfg.saturation = sensor->status.saturation;
        cfg.sharpness = sensor->status.sharpness;
        cfg.denoise = sensor->status.denoise;
        cfg.special_effect = sensor->status.special_effect;
        cfg.wb_mode = sensor->status.wb_mode;
        cfg.ae_level = sensor->status.ae_level;
        cfg.aec_value = sensor->status.aec_value;
        cfg.agc_gain = sensor->status.agc_gain;
        cfg.gainceiling = sensor->status.gainceiling;

        cfg.awb = sensor->status.awb;
        cfg.awb_gain = sensor->status.awb_gain;
        cfg.agc = sensor->status.agc;
        cfg.bpc = sensor->status.bpc;
        cfg.wpc = sensor->status.wpc;
        cfg.raw_gma = sensor->status.raw_gma;
        cfg.lenc = sensor->status.lenc;
        cfg.hmirror = sensor->status.hmirror;
        cfg.vflip = sensor->status.vflip;
        cfg.downsize = sensor->status.dcw;
        cfg.colorbar = sensor->status.colorbar;
        cfg.aec = sensor->status.aec;
        cfg.aec2 = sensor->status.aec2;
    }

#pragma region SETTER
    bool CameraController::setFrameSize(framesize_t size)
    {
        return (sensor && sensor->set_framesize) ? sensor->set_framesize(sensor, size) == 0 : false;
    }

    bool CameraController::setQuality(int value)
    {
        return (sensor && sensor->set_quality) ? sensor->set_quality(sensor, value) == 0 : false;
    }

    bool CameraController::setContrast(int level)
    {
        return (sensor && sensor->set_contrast) ? sensor->set_contrast(sensor, level) == 0 : false;
    }

    bool CameraController::setBrightness(int level)
    {
        return (sensor && sensor->set_brightness) ? sensor->set_brightness(sensor, level) == 0 : false;
    }

    bool CameraController::setSaturation(int level)
    {
        return (sensor && sensor->set_saturation) ? sensor->set_saturation(sensor, level) == 0 : false;
    }

    bool CameraController::setSharpness(int level)
    {
        return (sensor && sensor->set_sharpness) ? sensor->set_sharpness(sensor, level) == 0 : false;
    }

    bool CameraController::setDenoise(int level)
    {
        return (sensor && sensor->set_denoise) ? sensor->set_denoise(sensor, level) == 0 : false;
    }

    bool CameraController::setSpecialEffect(int effect)
    {
        return (sensor && sensor->set_special_effect) ? sensor->set_special_effect(sensor, effect) == 0 : false;
    }

    bool CameraController::setWbMode(int mode)
    {
        return (sensor && sensor->set_wb_mode) ? sensor->set_wb_mode(sensor, mode) == 0 : false;
    }

    bool CameraController::setAwb(bool enable)
    {
        return (sensor && sensor->set_whitebal) ? sensor->set_whitebal(sensor, enable) == 0 : false;
    }

    bool CameraController::setAwbGain(bool enable)
    {
        return (sensor && sensor->set_awb_gain) ? sensor->set_awb_gain(sensor, enable) == 0 : false;
    }

    bool CameraController::setAec(bool enable)
    {
        return (sensor && sensor->set_exposure_ctrl) ? sensor->set_exposure_ctrl(sensor, enable) == 0 : false;
    }

    bool CameraController::setAec2(bool enable)
    {
        return (sensor && sensor->set_aec2) ? sensor->set_aec2(sensor, enable) == 0 : false;
    }

    bool CameraController::setAecValue(int value)
    {
        return (sensor && sensor->set_aec_value) ? sensor->set_aec_value(sensor, value) == 0 : false;
    }

    bool CameraController::setAeLevel(int level)
    {
        return (sensor && sensor->set_ae_level) ? sensor->set_ae_level(sensor, level) == 0 : false;
    }

    bool CameraController::setAgc(bool enable)
    {
        return (sensor && sensor->set_gain_ctrl) ? sensor->set_gain_ctrl(sensor, enable) == 0 : false;
    }

    bool CameraController::setAgcGain(int value)
    {
        return (sensor && sensor->set_agc_gain) ? sensor->set_agc_gain(sensor, value) == 0 : false;
    }

    bool CameraController::setGainCeiling(int value)
    {
        return (sensor && sensor->set_gainceiling) ? sensor->set_gainceiling(sensor, (gainceiling_t)value) == 0 : false;
    }

    bool CameraController::setBpc(bool enable)
    {
        return (sensor && sensor->set_bpc) ? sensor->set_bpc(sensor, enable) == 0 : false;
    }

    bool CameraController::setWpc(bool enable)
    {
        return (sensor && sensor->set_wpc) ? sensor->set_wpc(sensor, enable) == 0 : false;
    }

    bool CameraController::setRawGma(bool enable)
    {
        return (sensor && sensor->set_raw_gma) ? sensor->set_raw_gma(sensor, enable) == 0 : false;
    }

    bool CameraController::setLensCorrection(bool enable)
    {
        return (sensor && sensor->set_lenc) ? sensor->set_lenc(sensor, enable) == 0 : false;
    }

    bool CameraController::setHorizontalMirror(bool enable)
    {
        return (sensor && sensor->set_hmirror) ? sensor->set_hmirror(sensor, enable) == 0 : false;
    }

    bool CameraController::setVerticalFlip(bool enable)
    {
        return (sensor && sensor->set_vflip) ? sensor->set_vflip(sensor, enable) == 0 : false;
    }

    bool CameraController::setDownsizeEnable(bool enable)
    {
        return (sensor && sensor->set_dcw) ? sensor->set_dcw(sensor, enable) == 0 : false;
    }

    bool CameraController::setColorBar(bool enable)
    {
        return (sensor && sensor->set_colorbar) ? sensor->set_colorbar(sensor, enable) == 0 : false;
    }

#pragma endregion SETTER

    __attribute__((noinline)) bool CameraController::load()
    {
        if (!persist::load_struct(CAMERA_CFG_PATH, &m_config, sizeof(m_config)))
        {
            ESP_LOGW(TAG, "No camera config found or corrupted");
            return false;
        }

        apply_config(m_config);
        ESP_LOGI(TAG, "Camera config applied from storage");
        return true;
    }
    __attribute__((noinline)) bool CameraController::save()
    {
        extract_config(m_config);
        return persist::save_struct(CAMERA_CFG_PATH, &m_config, sizeof(m_config));
    }

    __attribute__((noinline)) static void save_task(void *arg)
    {
        ESP_LOGI(TAG, "Démarrage tâche de sauvegarde AP");
        CameraController::getInstance().save();
        vTaskDelete(nullptr);
    }

    __attribute__((noinline)) void CameraController::request_save()
    {
        xTaskCreate(save_task, "cam_save", 4096, nullptr, 3, nullptr);
    }
    bool CameraController::from_json(const std::string &json)
    {
        if (json.empty() || json.size() > 1024)
            return false;

        cJSON *root = cJSON_Parse(json.c_str());
        if (!root)
            return false;

        cJSON *js = cJSON_GetObjectItemCaseSensitive(root, KEY_FRAME_SIZE);
        cJSON *jp = cJSON_GetObjectItemCaseSensitive(root, KEY_JPEG_QUALITY);
        cJSON *jc = cJSON_GetObjectItemCaseSensitive(root, KEY_CONTRAST);
        cJSON *jb = cJSON_GetObjectItemCaseSensitive(root, KEY_BRIGHTNESS);
        cJSON *ja = cJSON_GetObjectItemCaseSensitive(root, KEY_SATURATION);
        cJSON *je = cJSON_GetObjectItemCaseSensitive(root, KEY_AEC_VALUE);
        cJSON *jf = cJSON_GetObjectItemCaseSensitive(root, KEY_AE_LEVEL);
        cJSON *jg = cJSON_GetObjectItemCaseSensitive(root, KEY_AWB);
        cJSON *jh = cJSON_GetObjectItemCaseSensitive(root, KEY_AWB_GAIN);
        cJSON *ji = cJSON_GetObjectItemCaseSensitive(root, KEY_AGC);
        cJSON *jj = cJSON_GetObjectItemCaseSensitive(root, KEY_AGC_GAIN);
        cJSON *jk = cJSON_GetObjectItemCaseSensitive(root, KEY_BPC);
        cJSON *jl = cJSON_GetObjectItemCaseSensitive(root, KEY_WPC);
        cJSON *jm = cJSON_GetObjectItemCaseSensitive(root, KEY_RAW_GMA);
        cJSON *jn = cJSON_GetObjectItemCaseSensitive(root, KEY_LENC);
        cJSON *jo = cJSON_GetObjectItemCaseSensitive(root, KEY_HMIRROR);
        cJSON *jp2 = cJSON_GetObjectItemCaseSensitive(root, KEY_VFLIP);
        cJSON *jq = cJSON_GetObjectItemCaseSensitive(root, KEY_DOWNSIZE);
        cJSON *jr = cJSON_GetObjectItemCaseSensitive(root, KEY_COLORBAR);
        cJSON *jsharpness = cJSON_GetObjectItemCaseSensitive(root, KEY_SHARPNESS);
        cJSON *jdenoise = cJSON_GetObjectItemCaseSensitive(root, KEY_DENOISE);
        cJSON *jeffect = cJSON_GetObjectItemCaseSensitive(root, KEY_SPECIAL_EFFECT);
        cJSON *jwb = cJSON_GetObjectItemCaseSensitive(root, KEY_WB_MODE);
        cJSON *jaec = cJSON_GetObjectItemCaseSensitive(root, KEY_AEC);
        cJSON *jaec2 = cJSON_GetObjectItemCaseSensitive(root, KEY_AEC2);
        cJSON *jgainceil = cJSON_GetObjectItemCaseSensitive(root, KEY_GAINCEILING);

        if (cJSON_IsNumber(js) || cJSON_IsNumber(jp) || cJSON_IsNumber(jc) || cJSON_IsNumber(jb) || cJSON_IsNumber(ja) ||
            cJSON_IsNumber(je) || cJSON_IsNumber(jf) || cJSON_IsBool(jg) || cJSON_IsBool(jh) || cJSON_IsBool(ji) ||
            cJSON_IsNumber(jj) || cJSON_IsBool(jk) || cJSON_IsBool(jl) || cJSON_IsBool(jm) || cJSON_IsBool(jn) ||
            cJSON_IsBool(jo) || cJSON_IsBool(jp2) || cJSON_IsBool(jq) || cJSON_IsBool(jr) ||
            cJSON_IsNumber(jsharpness) || cJSON_IsNumber(jdenoise) ||
            cJSON_IsNumber(jeffect) || cJSON_IsNumber(jwb) || cJSON_IsBool(jaec) || cJSON_IsBool(jaec2) || cJSON_IsNumber(jgainceil))
        {

            auto fs = static_cast<framesize_t>(js->valueint);
            auto q = static_cast<int>(jp->valueint);
            auto c = static_cast<int>(jc->valueint);
            auto b = static_cast<int>(jb->valueint);
            auto s = static_cast<int>(ja->valueint);
            auto aec = static_cast<int>(je->valueint);
            auto ae = static_cast<int>(jf->valueint);
            auto awb = static_cast<bool>(jg->valueint);
            auto awb_gain = static_cast<bool>(jh->valueint);
            auto agc = static_cast<bool>(ji->valueint);
            auto agc_gain = static_cast<int>(jj->valueint);
            auto bpc = static_cast<bool>(jk->valueint);
            auto wpc = static_cast<bool>(jl->valueint);
            auto raw_gma = static_cast<bool>(jm->valueint);
            auto lenc = static_cast<bool>(jn->valueint);
            auto hmirror = static_cast<bool>(jo->valueint);
            auto vflip = static_cast<bool>(jp2->valueint);
            auto downsize = static_cast<bool>(jq->valueint);
            auto colorbar = static_cast<bool>(jr->valueint);
            auto sharpness = static_cast<int>(jsharpness->valueint);
            auto denoise = static_cast<int>(jdenoise->valueint);
            auto special_effect = static_cast<int>(jeffect->valueint);
            auto wb_mode = static_cast<int>(jwb->valueint);
            auto aec2 = static_cast<bool>(jaec2->valueint);
            auto gainceil = static_cast<int>(jgainceil->valueint);

            if (fs != sensor->status.framesize)
            {
                setFrameSize(fs);
            }
            if (q != sensor->status.quality)
            {
                setQuality(q);
            }
            if (c != sensor->status.contrast)
            {
                setContrast(c);
            }
            if (b != sensor->status.brightness)
            {
                setBrightness(b);
            }
            if (s != sensor->status.saturation)
            {
                setSaturation(s);
            }
            if (aec != sensor->status.aec_value)
            {
                setAecValue(aec);
            }
            if (ae != sensor->status.ae_level)
            {
                setAeLevel(ae);
            }
            if (awb != sensor->status.awb)
            {
                setAwb(awb);
            }
            if (awb_gain != sensor->status.awb_gain)
            {
                setAwbGain(awb_gain);
            }
            if (agc != sensor->status.agc)
            {
                setAgc(agc);
            }
            if (agc_gain != sensor->status.agc_gain)
            {
                setAgcGain(agc_gain);
            }
            if (bpc != sensor->status.bpc)
            {
                setBpc(bpc);
            }
            if (wpc != sensor->status.wpc)
            {
                setWpc(wpc);
            }
            if (raw_gma != sensor->status.raw_gma)
            {
                setRawGma(raw_gma);
            }
            if (lenc != sensor->status.lenc)
            {
                setLensCorrection(lenc);
            }
            if (hmirror != sensor->status.hmirror)
            {
                setHorizontalMirror(hmirror);
            }
            if (vflip != sensor->status.vflip)
            {
                setVerticalFlip(vflip);
            }
            if (downsize != sensor->status.denoise)
            {
                setDownsizeEnable(downsize);
            }
            if (colorbar != sensor->status.colorbar)
            {
                setColorBar(colorbar);
            }
            if (sharpness != sensor->status.sharpness)
            {
                setSharpness(sharpness);
            }
            if (denoise != sensor->status.denoise)
            {
                setDenoise(denoise);
            }
            if (special_effect != sensor->status.special_effect)
            {
                setSpecialEffect(special_effect);
            }
            if (wb_mode != sensor->status.wb_mode)
            {
                setWbMode(wb_mode);
            }
            if (aec2 != sensor->status.aec2)
            {
                setAec2(aec2);
            }
            if (gainceil != sensor->status.gainceiling)
            {
                setGainCeiling(gainceil);
            }
        }
        cJSON_Delete(root);
        ESP_LOGI(TAG, "Camera settings updated");
        return true;
    }

    __attribute__((noinline)) std::string CameraController::to_json()
    {
        if (!sensor)
        {
            ESP_LOGE(TAG, "sensor is null");
            return "";
        }

        cJSON *root = cJSON_CreateObject();
        if (!root)
        {
            ESP_LOGE(TAG, "cJSON_CreateObject failed");
            return "";
        }

        cJSON_AddNumberToObject(root, KEY_FRAME_SIZE, sensor->status.framesize);
        cJSON_AddNumberToObject(root, KEY_JPEG_QUALITY, sensor->status.quality);
        cJSON_AddNumberToObject(root, KEY_CONTRAST, sensor->status.contrast);
        cJSON_AddNumberToObject(root, KEY_BRIGHTNESS, sensor->status.brightness);
        cJSON_AddNumberToObject(root, KEY_SATURATION, sensor->status.saturation);
        cJSON_AddNumberToObject(root, KEY_AEC_VALUE, sensor->status.aec_value);
        cJSON_AddNumberToObject(root, KEY_AE_LEVEL, sensor->status.ae_level);
        cJSON_AddBoolToObject(root, KEY_AWB, sensor->status.awb);
        cJSON_AddBoolToObject(root, KEY_AWB_GAIN, sensor->status.awb_gain);
        cJSON_AddBoolToObject(root, KEY_AGC, sensor->status.agc);
        cJSON_AddNumberToObject(root, KEY_AGC_GAIN, sensor->status.agc_gain);
        cJSON_AddBoolToObject(root, KEY_BPC, sensor->status.bpc);
        cJSON_AddBoolToObject(root, KEY_WPC, sensor->status.wpc);
        cJSON_AddBoolToObject(root, KEY_RAW_GMA, sensor->status.raw_gma);
        cJSON_AddBoolToObject(root, KEY_LENC, sensor->status.lenc);
        cJSON_AddBoolToObject(root, KEY_HMIRROR, sensor->status.hmirror);
        cJSON_AddBoolToObject(root, KEY_VFLIP, sensor->status.vflip);
        cJSON_AddBoolToObject(root, KEY_DOWNSIZE, sensor->status.dcw);
        cJSON_AddBoolToObject(root, KEY_COLORBAR, sensor->status.colorbar);
        cJSON_AddNumberToObject(root, KEY_SHARPNESS, sensor->status.sharpness);
        cJSON_AddNumberToObject(root, KEY_DENOISE, sensor->status.denoise);
        cJSON_AddNumberToObject(root, KEY_SPECIAL_EFFECT, sensor->status.special_effect);
        cJSON_AddNumberToObject(root, KEY_WB_MODE, sensor->status.wb_mode);
        cJSON_AddBoolToObject(root, KEY_AEC, sensor->status.aec);
        cJSON_AddBoolToObject(root, KEY_AEC2, sensor->status.aec2);
        cJSON_AddNumberToObject(root, KEY_GAINCEILING, sensor->status.gainceiling);

        char *ret = cJSON_Print(root);
        cJSON_Delete(root);

        if (ret)
            return ret;
        else
            return "";
    }

}