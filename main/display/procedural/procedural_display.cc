// procedural/procedural_display.cc
#include "procedural_display.h"

#include <cmath>
#include <cstring>

#include "animation_library.h"
#include "eye_shape.h"

#include <esp_log.h>
#include <esp_log.h>
#include <esp_random.h>

#include <lvgl.h>

#define TAG "ProceduralDisplay"

#ifdef CONFIG_PROCEDURAL_DEBUG
#define PROCEDURAL_DEBUG(fmt, ...) ESP_LOGI(TAG, fmt, ##__VA_ARGS__)
#else
#define PROCEDURAL_DEBUG(fmt, ...)
#endif

namespace procedural {

ProceduralDisplay::ProceduralDisplay(esp_lcd_panel_io_handle_t panel_io,
                                     esp_lcd_panel_handle_t panel,
                                     int width, int height, int offset_x, int offset_y,
                                     bool mirror_x, bool mirror_y, bool swap_xy)
: SpiLcdDisplay(panel_io, panel, width, height, offset_x, offset_y,
    mirror_x, mirror_y, swap_xy) {
    ESP_LOGI(TAG, "ProceduralDisplay constructed %dx%d", width, height);

    EyeParameters neutral = EyeShape::PresetNeutral();
    neutral.visible = true;
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
}

void ProceduralDisplay::SetupUI() {
    ESP_LOGI(TAG, "SetupUI starting");
    if (IsSetupUICalled()) {
        ESP_LOGW(TAG, "SetupUI already called, skipping");
        return;
    }
    Display::SetupUI();
    DisplayLockGuard lock(this);

    ESP_LOGI(TAG, "SetupUI: %dx%d procedural face", width_, height_);

    auto screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    bg_rect_ = lv_obj_create(screen);
    lv_obj_set_size(bg_rect_, width_, height_);
    lv_obj_set_pos(bg_rect_, 0, 0);
    lv_obj_set_style_bg_color(bg_rect_, lv_color_black(), 0);
    lv_obj_set_style_border_width(bg_rect_, 0, 0);
    lv_obj_set_style_pad_all(bg_rect_, 0, 0);

    left_eye_ = CreateEyeObj(screen);
    right_eye_ = CreateEyeObj(screen);
    lv_obj_set_user_data(left_eye_, this);
    lv_obj_set_user_data(right_eye_, this);

    left_draw_.color = lv_color_make(0, 220, 220);
    right_draw_.color = lv_color_make(0, 220, 220);

    // Phase 2 fix: Hide eyes initially to prevent half-open first frame
    // They will be unhidden when bootclose clip runs (starts from closed)
    lv_obj_add_flag(left_eye_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(right_eye_, LV_OBJ_FLAG_HIDDEN);

    status_label_ = lv_label_create(screen);
    lv_obj_set_width(status_label_, width_);
    lv_obj_set_style_text_align(status_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(status_label_, lv_color_white(), 0);
    lv_label_set_text(status_label_, "");
    lv_obj_set_pos(status_label_, 0, height_ - 18);

    chat_label_ = lv_label_create(screen);
    lv_obj_set_width(chat_label_, width_ - 8);
    lv_obj_set_style_text_align(chat_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(chat_label_, lv_color_white(), 0);
    lv_label_set_long_mode(chat_label_, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_label_set_text(chat_label_, "");
    lv_obj_set_pos(chat_label_, 4, height_ - 36);
    lv_obj_add_flag(chat_label_, LV_OBJ_FLAG_HIDDEN);

    UpdateFaceGeometry();
    StartAnimTimer();
    PlayClip(AnimationLibrary::BootCloseFirst());
    lv_obj_invalidate(lv_screen_active());

    ESP_LOGI(TAG, "SetupUI complete");
}

void ProceduralDisplay::SetStatus(const char* status) {
    if (!status_label_ || !status) return;
    DisplayLockGuard lock(this);
    lv_label_set_text(status_label_, status);
}

void ProceduralDisplay::SetEmotion(const char* emotion) {
    if (!emotion) return;
    DisplayLockGuard lock(this);

    FaceState new_base = scheduler_.base();

    if (strcmp(emotion, "happy") == 0) {
        new_base.left = EyeShape::PresetHappy();
        new_base.right = EyeShape::PresetHappy();
        PlayClip(AnimationLibrary::Happy());
    } else if (strcmp(emotion, "sad") == 0) {
        new_base.left = EyeShape::PresetSad();
        new_base.right = EyeShape::PresetSad();
        PlayClip(AnimationLibrary::Sad());
    } else if (strcmp(emotion, "angry") == 0) {
        new_base.left = EyeShape::PresetAngry();
        new_base.right = EyeShape::PresetAngry();
        PlayClip(AnimationLibrary::Angry());
    } else if (strcmp(emotion, "surprised") == 0) {
        new_base.left = EyeShape::PresetSurprised();
        new_base.right = EyeShape::PresetSurprised();
        PlayClip(AnimationLibrary::Surprised());
    } else if (strcmp(emotion, "sleepy") == 0) {
        new_base.left = EyeShape::PresetSleepy();
        new_base.right = EyeShape::PresetSleepy();
        current_phase_ = FacePhase::SLEEPING;
        scheduler_.Stop("breathing");
        scheduler_.Stop("micro_tilt");
    } else if (strcmp(emotion, "neutral") == 0) {
        new_base.left = EyeShape::PresetNeutral();
        new_base.right = EyeShape::PresetNeutral();
        if (current_phase_ == FacePhase::SLEEPING) {
            current_phase_ = FacePhase::IDLE;
        }
    } else {
        new_base.left = EyeShape::PresetNeutral();
        new_base.right = EyeShape::PresetNeutral();
    }

    scheduler_.SetBaseState(new_base);
    current_face_ = new_base;
}

void ProceduralDisplay::SetChatMessage(const char* role, const char* content) {
    (void)role;
    if (!chat_label_ || !content) return;
    DisplayLockGuard lock(this);
    lv_label_set_text(chat_label_, content);
    lv_obj_remove_flag(chat_label_, LV_OBJ_FLAG_HIDDEN);
}

void ProceduralDisplay::ClearChatMessages() {
    if (!chat_label_) return;
    DisplayLockGuard lock(this);
    lv_label_set_text(chat_label_, "");
    lv_obj_add_flag(chat_label_, LV_OBJ_FLAG_HIDDEN);
}

void ProceduralDisplay::ShowNotification(const char* notification, int duration_ms) {
    if (!status_label_ || !notification) return;
    DisplayLockGuard lock(this);
    lv_label_set_text(status_label_, notification);

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
        DisplayLockGuard lock(self);
        lv_label_set_text(self->status_label_, "");
    }
}

void ProceduralDisplay::SetPowerSaveMode(bool on) {
    DisplayLockGuard lock(this);
    power_save_ = on;
    if (on) {
        lv_obj_set_style_line_opa(left_eye_, 40, 0);
        lv_obj_set_style_line_opa(right_eye_, 40, 0);
    } else {
        lv_obj_set_style_line_opa(left_eye_, LV_OPA_COVER, 0);
        lv_obj_set_style_line_opa(right_eye_, LV_OPA_COVER, 0);
    }
}

void ProceduralDisplay::UpdateStatusBar(bool update_all) {
    (void)update_all;
}

void ProceduralDisplay::SetFaceState(int phase_int) {
    if (phase_int < 0 || phase_int > 11) return;
    FacePhase phase = static_cast<FacePhase>(phase_int);
    DisplayLockGuard lock(this);
    current_phase_ = phase;
    PROCEDURAL_DEBUG("SetFaceState: %d", phase_int);

    FaceState new_base = scheduler_.base();

    // Stop all looped ambient behaviors first
    scheduler_.Stop("breathing");
    scheduler_.Stop("micro_tilt");
    scheduler_.Stop("sleepidle");
    scheduler_.Stop("microtiltswap");
    scheduler_.Stop("orbitsearch");
    scheduler_.Stop("speaking_shift");
    scheduler_.Stop("wander");

    // Reset behavior cooldowns on phase transition to prevent explosion
    scheduler_.ResetCooldowns();

    switch (phase) {
    case FacePhase::BOOTING:
        PlayClip(AnimationLibrary::BootCloseFirst());
        break;
    case FacePhase::IDLE:
        new_base.left = EyeShape::PresetNeutral();
        new_base.right = EyeShape::PresetNeutral();
        // Always restart ambient loops for IDLE
        PlayClip(AnimationLibrary::Breathing());
        PlayClip(AnimationLibrary::MicroTilt());
        PlayClip(AnimationLibrary::Wander());
        break;
    case FacePhase::LISTENING:
        new_base.left = EyeShape::PresetNeutral();
        new_base.right = EyeShape::PresetNeutral();
        PlayClip(AnimationLibrary::Breathing());
        PlayClip(AnimationLibrary::Wander());
        break;
    case FacePhase::THINKING:
        new_base.left = EyeShape::PresetNeutral();
        new_base.right = EyeShape::PresetNeutral();
        PlayClip(AnimationLibrary::Thinking());
        // Thinking has subtle micro-tilt
        PlayClip(AnimationLibrary::MicroTilt());
        break;
    case FacePhase::SPEAKING:
        new_base.left = EyeShape::PresetNeutral();
        new_base.right = EyeShape::PresetNeutral();
        // Speaking: active but controlled motion with micro-saccades and breathing
        PlayClip(AnimationLibrary::SpeakingShift());
        PlayClip(AnimationLibrary::Breathing());
        break;
    case FacePhase::SLEEPING:
        new_base.left = EyeShape::PresetSleepy();
        new_base.right = EyeShape::PresetSleepy();
        PlayClip(AnimationLibrary::SleepIdle());
        break;
    case FacePhase::WAKING:
        new_base.left = EyeShape::PresetNeutral();
        new_base.right = EyeShape::PresetNeutral();
        PlayClip(AnimationLibrary::WakeEnter());
        break;
    case FacePhase::HAPPY:
        new_base.left = EyeShape::PresetHappy();
        new_base.right = EyeShape::PresetHappy();
        PlayClip(AnimationLibrary::Happy());
        // Happy still has subtle ambient motion
        PlayClip(AnimationLibrary::Breathing());
        break;
    case FacePhase::SAD:
        new_base.left = EyeShape::PresetSad();
        new_base.right = EyeShape::PresetSad();
        PlayClip(AnimationLibrary::Sad());
        break;
    case FacePhase::ANGRY:
        new_base.left = EyeShape::PresetAngry();
        new_base.right = EyeShape::PresetAngry();
        PlayClip(AnimationLibrary::Angry());
        break;
    case FacePhase::CONFUSED:
        new_base.left = EyeShape::PresetNeutral();
        new_base.right = EyeShape::PresetNeutral();
        PlayClip(AnimationLibrary::MicroTilt());
        break;
    case FacePhase::ERROR_STATE:
        new_base.left = EyeShape::PresetAngry();
        new_base.right = EyeShape::PresetAngry();
        PlayClip(AnimationLibrary::Angry());
        break;
    default:
        new_base.left = EyeShape::PresetNeutral();
        new_base.right = EyeShape::PresetNeutral();
        PlayClip(AnimationLibrary::Breathing());
        break;
    }

    scheduler_.SetBaseState(new_base);
    current_face_ = new_base;
}

void ProceduralDisplay::PlayClip(const Clip* clip) {
    if (clip) {
        PROCEDURAL_DEBUG("PlayClip: %s", clip->name);
        scheduler_.Play(clip);
    }
}

void ProceduralDisplay::OnSpeechSentenceStart() {
    DisplayLockGuard lock(this);
    PROCEDURAL_DEBUG("sentence_start event");
    if (current_phase_ == FacePhase::SPEAKING) {
        PlayClip(AnimationLibrary::EmphasisBlink());
    }
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
    esp_timer_start_periodic(anim_timer_, 50 * 1000);
}

void ProceduralDisplay::AnimTimerCallback(void* arg) {
    auto* self = static_cast<ProceduralDisplay*>(arg);
    if (self) self->UpdateFromScheduler();
}

void ProceduralDisplay::UpdateFromScheduler() {
    DisplayLockGuard lock(this);
    uint32_t now_ms = lv_tick_get();

    if (current_phase_ == FacePhase::BOOTING && !scheduler_.IsPlaying("bootclose")) {
        SetFaceState(static_cast<int>(FacePhase::IDLE));
    }

    float now_sec = now_ms / 1000.0f;
    current_face_ = scheduler_.Update(now_sec, current_phase_);

    static uint32_t last_autonomous_ms = 0;
    uint32_t min_interval = 1500;
    uint32_t random_max = 1500;
    switch (current_phase_) {
        case FacePhase::IDLE:         min_interval = 1500; random_max = 1500; break;
        case FacePhase::LISTENING:   min_interval = 2000; random_max = 2000; break;
        case FacePhase::THINKING:    min_interval = 1500; random_max = 1000; break;
        case FacePhase::SPEAKING:     min_interval = 1000; random_max = 1000; break;
        case FacePhase::SLEEPING:   min_interval = 3000; random_max = 2000; break;
        default:                    min_interval = 1500; random_max = 1500; break;
    }
    if (now_ms - last_autonomous_ms > min_interval + (esp_random() % random_max)) {
        if (scheduler_.TryPlayAutonomous(current_phase_, now_ms)) {
            PROCEDURAL_DEBUG("autonomous behavior");
            last_autonomous_ms = now_ms;
        }
    }

    UpdateFaceGeometry();
}

namespace {

inline int Clamp(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

} // anonymous namespace

void ProceduralDisplay::UpdateFaceGeometry() {
    if (!left_eye_ || !right_eye_) return;

    const FaceState& s = current_face_;
    const FaceRigTransform& rig = s.rig;

    float display_cx = width_ / 2.0f;
    float display_cy = height_ / 2.0f;

    float separation = DEFAULT_EYE_SEPARATION * rig.eye_gap * rig.face_scale;
    float left_cx = display_cx - separation / 2.0f;
    float right_cx = display_cx + separation / 2.0f;
    float base_cy = DEFAULT_EYE_Y + rig.face_y * FACE_OFFSET_PX_PER_UNIT;

    left_cx += rig.face_x * FACE_OFFSET_PX_PER_UNIT;
    right_cx += rig.face_x * FACE_OFFSET_PX_PER_UNIT;

    float tilt_rad = rig.face_tilt * 3.14159f / 180.0f;
    float cos_t = cosf(tilt_rad), sin_t = sinf(tilt_rad);

    float lx0 = left_cx - display_cx;
    float ly0 = base_cy - display_cy;
    float left_cx_final = display_cx + (lx0 * cos_t - ly0 * sin_t);
    float left_cy_final = display_cy + (lx0 * sin_t + ly0 * cos_t);

    float rx0 = right_cx - display_cx;
    float ry0 = base_cy - display_cy;
    float right_cx_final = display_cx + (rx0 * cos_t - ry0 * sin_t);
    float right_cy_final = display_cy + (rx0 * sin_t + ry0 * cos_t);

    constexpr int kEyeWidth = 44;
    constexpr int kEyeHeight = 64;

    EyeParameters left_eye = s.left;
    left_eye.rotation_deg += rig.face_tilt;

    EyeShape::Polygon lpoly = EyeShape::GenerateContour(
        left_eye, static_cast<int>(left_cx_final), static_cast<int>(left_cy_final), kEyeWidth, kEyeHeight);

    left_draw_.count = 0;
    for (uint8_t i = 0; i < lpoly.count && i < 16; ++i) {
        left_draw_.points[i].x = Clamp(lpoly.points[i].x, 0, width_ - 1);
        left_draw_.points[i].y = Clamp(lpoly.points[i].y, 0, height_ - 1);
        left_draw_.count++;
    }
    left_draw_.opa = static_cast<lv_opa_t>(s.left.brightness * 255);

    EyeParameters right_eye = s.right;
    right_eye.rotation_deg += rig.face_tilt;

    EyeShape::Polygon rpoly = EyeShape::GenerateContour(
        right_eye, static_cast<int>(right_cx_final), static_cast<int>(right_cy_final), kEyeWidth, kEyeHeight);

    right_draw_.count = 0;
    for (uint8_t i = 0; i < rpoly.count && i < 16; ++i) {
        right_draw_.points[i].x = Clamp(rpoly.points[i].x, 0, width_ - 1);
        right_draw_.points[i].y = Clamp(rpoly.points[i].y, 0, height_ - 1);
        right_draw_.count++;
    }
    right_draw_.opa = static_cast<lv_opa_t>(s.right.brightness * 255);

    if (s.left.visible && left_draw_.count > 2) {
        lv_obj_remove_flag(left_eye_, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(left_eye_, LV_OBJ_FLAG_HIDDEN);
    }

    if (s.right.visible && right_draw_.count > 2) {
        lv_obj_remove_flag(right_eye_, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(right_eye_, LV_OBJ_FLAG_HIDDEN);
    }

    lv_obj_invalidate(left_eye_);
    lv_obj_invalidate(right_eye_);
}

lv_obj_t* ProceduralDisplay::CreateEyeObj(lv_obj_t* parent) {
    lv_obj_t* obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(obj, EyeDrawEventCb, LV_EVENT_DRAW_MAIN, nullptr);
    return obj;
}

void ProceduralDisplay::EyeDrawEventCb(lv_event_t* e) {
    lv_obj_t* obj = static_cast<lv_obj_t*>(lv_event_get_target(e));
    lv_layer_t* layer = static_cast<lv_layer_t*>(lv_event_get_param(e));

    auto* disp = static_cast<ProceduralDisplay*>(lv_obj_get_user_data(obj));
    if (!disp || !layer) return;

    if (obj == disp->left_eye_) {
        DrawFilledPolygon(layer, disp->left_draw_, disp->width_, disp->height_);
    } else if (obj == disp->right_eye_) {
        DrawFilledPolygon(layer, disp->right_draw_, disp->width_, disp->height_);
    }
}

void ProceduralDisplay::DrawFilledPolygon(lv_layer_t* layer, const EyeDrawData& data, int max_width, int max_height) {
    if (data.count < 3) return;

    int16_t min_y = 32767, max_y = -32768;
    for (uint8_t i = 0; i < data.count; ++i) {
        if (data.points[i].y < min_y) min_y = data.points[i].y;
        if (data.points[i].y > max_y) max_y = data.points[i].y;
    }
    if (min_y > max_y) return;

    int16_t y_start = (min_y < 0) ? 0 : min_y;
    int16_t y_end = (max_y >= max_height) ? max_height - 1 : max_y;

    struct ScanlineIntersection { int16_t x; int16_t dir; };
    ScanlineIntersection intersects[32];

    for (int16_t y = y_start; y <= y_end; ++y) {
        uint8_t intersect_count = 0;

        for (uint8_t i = 0; i < data.count; ++i) {
            uint8_t next = (i + 1) % data.count;
            int16_t y0 = data.points[i].y;
            int16_t y1 = data.points[next].y;
            int16_t x0 = data.points[i].x;
            int16_t x1 = data.points[next].x;

            if ((y0 <= y && y1 > y) || (y1 <= y && y0 > y)) {
                float slope = float(x1 - x0) / float(y1 - y0);
                float x_inter = x0 + slope * float(y - y0);
                intersects[intersect_count++] = { int16_t(x_inter), 0 };
            }
        }

        if (intersect_count < 2) continue;

        for (uint8_t i = 0; i < intersect_count - 1; ++i) {
            for (uint8_t j = i + 1; j < intersect_count; ++j) {
                if (intersects[i].x > intersects[j].x) {
                    auto temp = intersects[i];
                    intersects[i] = intersects[j];
                    intersects[j] = temp;
                }
            }
        }

        for (uint8_t i = 0; i + 1 < intersect_count; i += 2) {
            int16_t x1 = Clamp(intersects[i].x, 0, max_width - 1);
            int16_t x2 = Clamp(intersects[i + 1].x, 0, max_width - 1);
            if (x2 <= x1) continue;

            lv_draw_line_dsc_t line_dsc;
            lv_draw_line_dsc_init(&line_dsc);
            line_dsc.p1 = { x1, y };
            line_dsc.p2 = { x2, y };
            line_dsc.color = data.color;
            line_dsc.opa = data.opa;
            line_dsc.width = 1;

            lv_draw_line(layer, &line_dsc);
        }
    }
}

} // namespace procedural
