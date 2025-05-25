#pragma once
#ifndef __IOT_CAMERA_API_H__
#define __IOT_CAMERA_API_H__

#include "esp_http_server.h"
#include "camera_controller.h"
#include "camera.h"

namespace camera_api
{
    static constexpr const char *TAG = "[camera_api]";
    __attribute__((noinline)) esp_err_t camera_get(httpd_req_t *req);
    __attribute__((noinline)) esp_err_t camera_post(httpd_req_t *req);
    __attribute__((noinline)) esp_err_t camera_save(httpd_req_t *req);
}

#endif