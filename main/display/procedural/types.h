#pragma once
#include <cstdint>
#include <cmath>

namespace procedural {

enum class EasingType { LINEAR, EASE_IN_QUAD, EASE_OUT_QUAD, EASE_IN_OUT_QUAD, EASE_OUT_BOUNCE };
enum class FaceEvent { NONE, WAKE_WORD, SPEECH_STARTED, SPEECH_STOPPED, PLAYBACK_STARTED, PLAYBACK_FINISHED, ERROR, SLEEP, WAKE, USER_ACTIVITY };
enum class FacePhase { BOOTING, IDLE, LISTENING, THINKING, SPEAKING, SLEEPING, WAKING, HAPPY, SAD, ANGRY, CONFUSED, ERROR_STATE };

struct EyeParameters {
    float center_x = 0.0f, center_y = 0.0f, scale_x = 1.0f, scale_y = 1.0f;
    float rotation_deg = 0.0f, inner_corner_raise = 0.0f, outer_corner_raise = 0.0f;
    float top_cut = 0.0f, bottom_cut = 0.0f, inner_tension = 0.5f, outer_tension = 0.5f;
    float roundness = 0.6f, brightness = 1.0f;
    bool visible = true;
};

struct FaceRigTransform {
    float face_warp = 0.0f, face_x = 0.0f, face_y = 0.0f, face_scale = 1.0f;
};

struct FaceState {
    EyeParameters left, right;
    FaceRigTransform rig;
};

struct Point { int16_t x, y; };
struct Rect { int16_t x, y, w, h; };

struct KeyFrame {
    float time, value;
    EasingType easing;
    bool operator<(const KeyFrame& o) const { return time < o.time; }
};

struct Clip {
    const char* name;
    float duration;
    uint8_t priority;
    bool loop, interruptible;
    KeyFrame* tracks;
    uint8_t track_count, kf_per_track;
};

} // namespace procedural