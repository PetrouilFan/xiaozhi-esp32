// procedural/procedural_display.cc
#include "procedural_display.h"
#include "eye_shape.h"
#include "animation_library.h"
#include <esp_log.h>
#include <esp_random.h>
#include <cstring>
#include <math.h>

#define TAG "ProceduralDisplay"

namespace procedural {

ProceduralDisplay::ProceduralDisplay(esp_lcd_panel_io_handle_t panel_io,
                                     esp_lcd_panel_handle_t panel,
                                     int width, int height)
    : panel_io_(panel_io), panel_(panel), width_(width), height_(height) {
    mutex_ = xSemaphoreCreateMutex();

    EyeParameters neutral = EyeShape::PresetNeutral();
    current_face_.left = neutral;
    current_face_.right = neutral;
    scheduler_.SetBaseState(current_face_);
}

ProceduralDisplay::~ProceduralDisplay() {
    if (anim_timer_) {
        esp_timer_stop(anim_timer_);
        esp_timer_delete(anim_timer_);
    }
    if (notif_timer_) {
        esp_timer_stop(notif_timer_);
        esp_timer_delete(notif_timer_);
    }
    if (mutex_) vSemaphoreDelete(mutex_);
}

bool ProceduralDisplay::Lock(int timeout_ms) {
    return xSemaphoreTake(mutex_, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}

void ProceduralDisplay::Unlock() {
    xSemaphoreGive(mutex_);
}

void ProceduralDisplay::SetupUI() {
    if (setup_ui_called_) return;
    setup_ui_called_ = true;

    ESP_LOGI(TAG, "SetupUI: %dx%d procedural face", width_, height_);

    // Dark background
    bg_rect_ = lv_obj_create(lv_screen_active());
    lv_obj_set_size(bg_rect_, width_, height_);
    lv_obj_set_pos(bg_rect_, 0, 0);
    lv_obj_set_style_bg_color(bg_rect_, lv_color_black(), 0);
    lv_obj_set_style_border_width(bg_rect_, 0, 0);
    lv_obj_set_style_pad_all(bg_rect_, 0, 0);

    // Left eye line
    left_eye_ = lv_line_create(lv_screen_active());
    lv_obj_set_style_line_width(left_eye_, 3, 0);
    lv_obj_set_style_line_color(left_eye_, lv_color_make(0, 220, 220), 0);
    lv_obj_set_style_line_rounded(left_eye_, true, 0);

    // Right eye line
    right_eye_ = lv_line_create(lv_screen_active());
    lv_obj_set_style_line_width(right_eye_, 3, 0);
    lv_obj_set_style_line_color(right_eye_, lv_color_make(0, 220, 220), 0);
    lv_obj_set_style_line_rounded(right_eye_, true, 0);

    // Status label at bottom
    status_label_ = lv_label_create(lv_screen_active());
    lv_obj_set_width(status_label_, width_);
    lv_obj_set_style_text_align(status_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(status_label_, lv_color_white(), 0);
    // Font not needed; LVGL default is fine for status bar
    lv_label_set_text(status_label_, "");
    lv_obj_set_pos(status_label_, 0, height_ - 18);

    // Chat label
    chat_label_ = lv_label_create(lv_screen_active());
    lv_obj_set_width(chat_label_, width_ - 8);
    lv_obj_set_style_text_align(chat_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(chat_label_, lv_color_white(), 0);
    lv_label_set_long_mode(chat_label_, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_label_set_text(chat_label_, "");
    lv_obj_set_pos(chat_label_, 4, height_ - 36);
    lv_obj_add_flag(chat_label_, LV_OBJ_FLAG_HIDDEN);

    // Start animation timer at 20 FPS
    StartAnimTimer();

    // Play boot animation
    PlayClip(AnimationLibrary::BootOpen());

    ESP_LOGI(TAG, "SetupUI complete");
}

void ProceduralDisplay::StartAnimTimer() {
    esp_timer_create_args_t timer_args = {
        .callback = AnimTimerCallback,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "procedural_anim",
        .skip_unhandled_events = true,
    };
    esp_timer_create(&timer_args, &anim_timer_);
    esp_timer_start_periodic(anim_timer_, 50 * 1000); // 50ms = 20 FPS
}

void ProceduralDisplay::AnimTimerCallback(void* arg) {
    auto* self = static_cast<ProceduralDisplay*>(arg);
    if (self) self->UpdateFromScheduler();
}

void ProceduralDisplay::UpdateFromScheduler() {
    if (!Lock(0)) return;

    uint32_t now_ms = lv_tick_get();
    float now_sec = now_ms / 1000.0f;

    // Update scheduler
    current_face_ = scheduler_.Update(now_sec);

    // Random blink if idle
    static uint32_t last_blink_ms = 0;
    if (current_phase_ == FacePhase::IDLE || current_phase_ == FacePhase::LISTENING) {
        if (now_ms - last_blink_ms > 4000 + (esp_random() % 3000)) {
            if (!scheduler_.IsPlaying("blink")) {
                PlayClip(AnimationLibrary::Blink());
                last_blink_ms = now_ms;
            }
        }
    }

    // Update eye geometry
    UpdateFaceGeometry();
    Unlock();
}

void ProceduralDisplay::UpdateFaceGeometry() {
    if (!left_eye_ || !right_eye_) return;

    int16_t eye_y = height_ / 2 - 10;
    int16_t eye_sep = width_ * 2 / 5;
    int16_t left_cx = (width_ - eye_sep) / 2;
    int16_t right_cx = (width_ + eye_sep) / 2;

    // Generate and set left eye points
    EyeShape::Polygon lpoly = EyeShape::GenerateContour(
        current_face_.left, left_cx, eye_y, 44, 56);
    uint8_t lcount = 0;
    for (uint8_t i = 0; i < lpoly.count && i < 16; ++i) {
        left_points_[i].x = lpoly.points[i].x;
        left_points_[i].y = lpoly.points[i].y;
        lcount++;
    }
    // Close the polygon
    if (lcount > 0) {
        left_points_[lcount] = left_points_[0];
        lcount++;
    }
    lv_line_set_points(left_eye_, left_points_, lcount);

    // Brightness affects opacity
    uint8_t lbri = (uint8_t)(current_face_.left.brightness * 255);
    lv_obj_set_style_line_opa(left_eye_, lbri, 0);

    // Generate and set right eye points
    EyeShape::Polygon rpoly = EyeShape::GenerateContour(
        current_face_.right, right_cx, eye_y, 44, 56);
    uint8_t rcount = 0;
    for (uint8_t i = 0; i < rpoly.count && i < 16; ++i) {
        right_points_[i].x = rpoly.points[i].x;
        right_points_[i].y = rpoly.points[i].y;
        rcount++;
    }
    if (rcount > 0) {
        right_points_[rcount] = right_points_[0];
        rcount++;
    }
    lv_line_set_points(right_eye_, right_points_, rcount);

    uint8_t rbri = (uint8_t)(current_face_.right.brightness * 255);
    lv_obj_set_style_line_opa(right_eye_, rbri, 0);

    // Visibility
    if (current_face_.left.visible && lcount > 1) {
        lv_obj_remove_flag(left_eye_, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(left_eye_, LV_OBJ_FLAG_HIDDEN);
    }
    if (current_face_.right.visible && rcount > 1) {
        lv_obj_remove_flag(right_eye_, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(right_eye_, LV_OBJ_FLAG_HIDDEN);
    }
}

void ProceduralDisplay::SetStatus(const char* status) {
    if (!status_label_ || !status) return;
    if (!Lock(50)) return;
    lv_label_set_text(status_label_, status);
    Unlock();
}

void ProceduralDisplay::SetEmotion(const char* emotion) {
    if (!emotion) return;
    if (!Lock(50)) return;

    if (strcmp(emotion, "happy") == 0) {
        current_face_.left = EyeShape::PresetHappy();
        current_face_.right = EyeShape::PresetHappy();
        PlayClip(AnimationLibrary::Happy());
    } else if (strcmp(emotion, "sad") == 0) {
        current_face_.left = EyeShape::PresetSad();
        current_face_.right = EyeShape::PresetSad();
        PlayClip(AnimationLibrary::Sad());
    } else if (strcmp(emotion, "angry") == 0) {
        current_face_.left = EyeShape::PresetAngry();
        current_face_.right = EyeShape::PresetAngry();
    } else if (strcmp(emotion, "surprised") == 0) {
        current_face_.left = EyeShape::PresetSurprised();
        current_face_.right = EyeShape::PresetSurprised();
    } else if (strcmp(emotion, "sleepy") == 0) {
        current_face_.left = EyeShape::PresetSleepy();
        current_face_.right = EyeShape::PresetSleepy();
    } else {
        current_face_.left = EyeShape::PresetNeutral();
        current_face_.right = EyeShape::PresetNeutral();
    }
    Unlock();
}

void ProceduralDisplay::SetChatMessage(const char* role, const char* content) {
    if (!chat_label_ || !content) return;
    if (!Lock(50)) return;
    lv_label_set_text(chat_label_, content);
    lv_obj_remove_flag(chat_label_, LV_OBJ_FLAG_HIDDEN);
    Unlock();
}

void ProceduralDisplay::ClearChatMessages() {
    if (!chat_label_) return;
    if (!Lock(50)) return;
    lv_label_set_text(chat_label_, "");
    lv_obj_add_flag(chat_label_, LV_OBJ_FLAG_HIDDEN);
    Unlock();
}

void ProceduralDisplay::ShowNotification(const char* notification, int duration_ms) {
    if (!status_label_ || !notification) return;
    if (!Lock(50)) return;
    lv_label_set_text(status_label_, notification);
    Unlock();

    if (duration_ms > 0) {
        esp_timer_create_args_t args = {
            .callback = NotifTimerCallback,
            .arg = this,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "notif_clear",
            .skip_unhandled_events = false,
        };
        if (notif_timer_) {
            esp_timer_stop(notif_timer_);
            esp_timer_delete(notif_timer_);
        }
        esp_timer_create(&args, &notif_timer_);
        esp_timer_start_once(notif_timer_, duration_ms * 1000);
    }
}

void ProceduralDisplay::NotifTimerCallback(void* arg) {
    auto* self = static_cast<ProceduralDisplay*>(arg);
    if (self && self->status_label_) {
        lv_label_set_text(self->status_label_, "");
    }
}

void ProceduralDisplay::SetPowerSaveMode(bool on) {
    if (!Lock(50)) return;
    power_save_ = on;
    if (on) {
        lv_obj_set_style_line_opa(left_eye_, 40, 0);
        lv_obj_set_style_line_opa(right_eye_, 40, 0);
    } else {
        lv_obj_set_style_line_opa(left_eye_, LV_OPA_COVER, 0);
        lv_obj_set_style_line_opa(right_eye_, LV_OPA_COVER, 0);
    }
    Unlock();
}

void ProceduralDisplay::UpdateStatusBar(bool update_all) {
    (void)update_all;
}

void ProceduralDisplay::SetFaceState(FacePhase phase) {
    if (!Lock(50)) return;
    current_phase_ = phase;
    switch (phase) {
        case FacePhase::BOOTING:
            PlayClip(AnimationLibrary::BootOpen());
            break;
        case FacePhase::LISTENING:
            current_face_.left = EyeShape::PresetFocused();
            current_face_.right = EyeShape::PresetFocused();
            break;
        case FacePhase::THINKING:
            PlayClip(AnimationLibrary::Thinking());
            break;
        case FacePhase::SPEAKING:
            current_face_.left = EyeShape::PresetNeutral();
            current_face_.right = EyeShape::PresetNeutral();
            break;
        case FacePhase::SLEEPING:
            PlayClip(AnimationLibrary::SleepEnter());
            break;
        case FacePhase::WAKING:
            PlayClip(AnimationLibrary::WakeEnter());
            break;
        case FacePhase::IDLE:
        default:
            current_face_.left = EyeShape::PresetNeutral();
            current_face_.right = EyeShape::PresetNeutral();
            break;
    }
    Unlock();
}

void ProceduralDisplay::PlayClip(const Clip* clip) {
    if (clip) scheduler_.Play(clip);
}

} // namespace procedural
