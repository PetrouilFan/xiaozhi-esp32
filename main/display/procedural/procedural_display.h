// procedural/procedural_display.h
// Robot face display with filled polygon eye rendering
#pragma once

#include "lcd_display.h"
#include "types.h"
#include "scheduler.h"
#include "eye_shape.h"

#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_timer.h>
#include <lvgl.h>

namespace procedural {

class ProceduralDisplay : public SpiLcdDisplay {
public:
    ProceduralDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel,
                      int width, int height, int offset_x, int offset_y,
                      bool mirror_x, bool mirror_y, bool swap_xy);
    ~ProceduralDisplay();

    virtual void SetupUI() override;
    virtual void SetStatus(const char* status) override;
    virtual void SetEmotion(const char* emotion) override;
    virtual void SetChatMessage(const char* role, const char* content) override;
    virtual void ClearChatMessages() override;
    virtual void ShowNotification(const char* notification, int duration_ms = 3000) override;
    virtual void SetPowerSaveMode(bool on) override;
    virtual void UpdateStatusBar(bool update_all = false) override;

    void SetFaceState(FacePhase phase);
    void PlayClip(const Clip* clip);

    struct EyeDrawData {
        lv_point_precise_t points[16];
        uint8_t count = 0;
        lv_color_t color;
        lv_opa_t opa = LV_OPA_COVER;
    };

protected:

private:
    static constexpr float DEFAULT_EYE_SEPARATION = 60.0f;
    static constexpr float FACE_OFFSET_PX_PER_UNIT = 20.0f;
    static constexpr int DEFAULT_EYE_Y = 120;

    lv_obj_t* left_eye_ = nullptr;
    lv_obj_t* right_eye_ = nullptr;
    EyeDrawData left_draw_;
    EyeDrawData right_draw_;

    lv_obj_t* status_label_ = nullptr;
    lv_obj_t* chat_label_ = nullptr;

    lv_obj_t* bg_rect_ = nullptr;

    esp_timer_handle_t anim_timer_ = nullptr;

    FacePhase current_phase_ = FacePhase::BOOTING;
    FaceState current_face_;
    Scheduler scheduler_;

    esp_timer_handle_t notif_timer_ = nullptr;

    bool power_save_ = false;

    void UpdateFaceGeometry();
    void StartAnimTimer();
    void UpdateFromScheduler();
    static void AnimTimerCallback(void* arg);
    static void NotifTimerCallback(void* arg);
    static void EyeDrawEventCb(lv_event_t* e);
    static lv_obj_t* CreateEyeObj(lv_obj_t* parent);
    static void DrawFilledPolygon(lv_layer_t* layer, const EyeDrawData& data);
};

} // namespace procedural
