#include "camera_api.h"
#include "../macro.h"
#include <string>
#include "esp_log.h"

namespace camera_api
{
    __attribute__((noinline)) esp_err_t camera_get(httpd_req_t *req)
    {
        std::string json = camera_controller::CameraController::getInstance().to_json();
        SET_RESP_HEADERS(req);
        return httpd_resp_send(req, json.c_str(), json.size());
    }
    __attribute__((noinline)) esp_err_t camera_post(httpd_req_t *req)
    {
        auto &cam = camera_controller::CameraController::getInstance();
        if (!cam.isInitialized())
            return httpd_resp_send_500(req);

        // int len = req->content_len;
        // std::string body(len, '\0');
        // if (httpd_req_recv(req, &body[0], len) <= 0)
        // {
        //     ESP_LOGE("[camera_api]", "wifi_ap_post: réception body failed");
        //     httpd_resp_send_500(req);
        //     return ESP_FAIL;

        HTTP_RECV_BODY_OR_FAIL(req, body, TAG, "body failed"); // }

        bool posted = camera_controller::CameraController::getInstance().from_json(body.c_str());
        if (!posted)
        {
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        SET_RESP_HEADERS(req);
        httpd_resp_send(req, body.c_str(), body.size());

        return ESP_OK;
    }

    __attribute__((noinline)) esp_err_t camera_save(httpd_req_t *req)
    {
        auto &cam = camera_controller::CameraController::getInstance();
        if (!cam.isInitialized())
            return httpd_resp_send_500(req);

        HTTP_RECV_BODY_OR_FAIL(req, body, TAG, "body failed");

        camera_controller::CameraController::getInstance().request_save();
        SET_RESP_HEADERS(req);

        httpd_resp_send(req, body.c_str(), body.size());

        return ESP_OK;
    }

}
