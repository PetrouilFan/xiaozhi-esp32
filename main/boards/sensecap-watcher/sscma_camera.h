#ifndef SSCMA_CAMERA_H
#define SSCMA_CAMERA_H

#include <cstdint>
#include <lvgl.h>
#include <thread>
#include <memory>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <esp_io_expander_tca95xx_16bit.h>
#include <esp_jpeg_dec.h>
#include <mbedtls/base64.h>

#include "sscma_client.h"
#include "camera.h"

struct SscmaData {
    uint8_t* img;
    size_t len;
};
struct JpegData {
    uint8_t* buf;
    size_t len;
};

class SscmaCamera : public Camera {
private:
    lv_img_dsc_t preview_image_;
    std::string explain_url_;
    std::string explain_token_;
    sscma_client_io_handle_t sscma_client_io_handle_;
    sscma_client_handle_t sscma_client_handle_;
    QueueHandle_t sscma_data_queue_;
    JpegData jpeg_data_;
    jpeg_dec_handle_t jpeg_dec_;
    jpeg_dec_io_t *jpeg_io_;
    jpeg_dec_header_info_t *jpeg_out_;
    // DetectStatus机
    enum DetectionState {
        IDLE,           // 空闲Status
        VALIDATING,     // Verify中（连续Detect3Second）
        COOLDOWN        // 冷却期（Waiting重新Detect）
    };
    
    DetectionState detection_state = IDLE;
    int64_t state_start_time = 0;
    bool need_start_cooldown = false; // 是否需要开始冷却期
    int64_t last_detected_time = 0; // Verify期间最后一次Detect到物体的Time
    
    int detect_target = 0;
    int detect_threshold = 75;
    int detect_duration_sec = 2; // Detect持续Time2Second，Confirm人员持续存在
    int detect_invoke_interval_sec = 8; // 默认15Second冷却期，避免频繁开始会话
    int detect_debounce_sec = 1; // Verify期间人员Leave的去抖动Time1Second
    int inference_en = 0; // 推理Enable开关（0: Close, 1: Open/Enable）
    bool sscma_restarted_ = false;
    
    sscma_client_model_t *model;
    int model_class_cnt = 0;
public:
    SscmaCamera(esp_io_expander_handle_t io_exp_handle);
    ~SscmaCamera();
    void InitializeMcpTools();

    virtual void SetExplainUrl(const std::string& url, const std::string& token);
    virtual bool Capture();
    // 翻转控制Functions
    virtual bool SetHMirror(bool enabled) override;
    virtual bool SetVFlip(bool enabled) override;
    virtual std::string Explain(const std::string& question);

};

#endif // ESP32_CAMERA_H
