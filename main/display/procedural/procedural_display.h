// procedural/procedural_display.h
// Robot face display using lv_line for wireframe eye rendering
#pragma once

#include "display.h"
#include "types.h"
#include "scheduler.h"

#include <lvgl.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_timer.h>

namespace procedural {

class ProceduralDisplay : public Display {
public:
    ProceduralDisplay(esp_lcd_panel_io_handle_t panel_io,
                      esp_lcd_panel_handle_t panel,
                      int width, int height);
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

protected:
    virtual bool Lock(int timeout_ms = 0) override;
    virtual void Unlock() override;

private:
    esp_lcd_panel_io_handle_t panel_io_;
    esp_lcd_panel_handle_t panel_;
    int width_, height_;

    // LVGL line objects for eye rendering
    lv_obj_t* left_eye_ = nullptr;
    lv_obj_t* right_eye_ = nullptr;
    lv_point_precise_t left_points_[17];  // 16 points + wrap-around
    lv_point_precise_t right_points_[17];

    // Status label at bottom
    lv_obj_t* status_label_ = nullptr;
    lv_obj_t* chat_label_ = nullptr;

    // Background rect for dark theme
    lv_obj_t* bg_rect_ = nullptr;

    // Animation timer
    esp_timer_handle_t anim_timer_ = nullptr;
    uint32_t last_update_ms_ = 0;

    // Face state
    FacePhase current_phase_ = FacePhase::BOOTING;
    FaceState current_face_;
    Scheduler scheduler_;

    // Notification timer
    esp_timer_handle_t notif_timer_ = nullptr;

    SemaphoreHandle_t mutex_;
    bool power_save_ = false;

    void UpdateFaceGeometry();
    void StartAnimTimer();
    void UpdateFromScheduler();
    static void AnimTimerCallback(void* arg);
    static void NotifTimerCallback(void* arg);
};

} // namespace procedural
