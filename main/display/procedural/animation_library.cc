// procedural/animation_library.cc
#include "animation_library.h"
#include <cstring>
#include <cmath>
#include "timeline.h"

namespace procedural {

// ==================== Simple (non-timeline) clips ====================

// Keyframe storage
static KeyFrame blink_kf[8];
static KeyFrame boot_kf[4];
static KeyFrame happy_kf[8];
static KeyFrame sad_kf[8];
static KeyFrame surprised_kf[4];
static KeyFrame thinking_kf[4];
static KeyFrame sleep_kf[6];
static KeyFrame wake_kf[6];

// Simple clip objects (no timeline)
static Clip clip_blink     = {"blink",    0.25f, 5, false, true,  blink_kf,    2, 3, nullptr};
static Clip clip_boot      = {"boot_open",0.8f, 10, false, false, boot_kf,    2, 2, nullptr};
static Clip clip_happy     = {"happy",   0.5f, 3, false, true,  happy_kf,    2, 3, nullptr};
static Clip clip_sad       = {"sad",     0.6f, 3, false, true,  sad_kf,      2, 3, nullptr};
static Clip clip_surprised = {"surprised",0.4f, 3, false, true,  surprised_kf,2, 2, nullptr};
static Clip clip_thinking  = {"thinking",0.5f, 3, false, true,  thinking_kf, 2, 2, nullptr};
static Clip clip_sleep     = {"sleep_enter",1.0f, 5, false, false, sleep_kf,    2, 3, nullptr};
static Clip clip_wake      = {"wake_enter",0.8f, 5, false, false, wake_kf,    2, 3, nullptr};

// ==================== Timeline support ====================

static bool timelines_initialized = false;
static Timeline timeline_angry;
static Timeline timeline_happy_hop;
static Timeline timeline_double_take_soft;
static Timeline timeline_breathing;
static Timeline timeline_micro_tilt;
static Timeline timeline_slowblink;
static Timeline timeline_doubleblink;
static Timeline timeline_sleepidle;
static Timeline timeline_test_horizontal;

// --- Angry clip keyframes (16 tracks) ---
static KeyFrame angry_left_inner_corner_raise[] = {
    {0.00f, 0.0f, EasingType::LINEAR}, {0.25f, -0.35f, EasingType::EASE_OUT_CUBIC},
    {0.75f, -0.33f}, {1.00f, 0.0f, EasingType::EASE_IN_CUBIC}
};
static KeyFrame angry_left_outer_corner_raise[] = {
    {0.00f, 0.0f, EasingType::LINEAR}, {0.25f, 0.15f, EasingType::EASE_OUT_CUBIC},
    {0.75f, 0.13f}, {1.00f, 0.0f, EasingType::EASE_IN_CUBIC}
};
static KeyFrame angry_left_top_cut[] = {
    {0.00f, 0.10f, EasingType::LINEAR}, {0.25f, 0.30f, EasingType::EASE_OUT_CUBIC},
    {0.75f, 0.28f}, {1.00f, 0.10f, EasingType::EASE_IN_CUBIC}
};
static KeyFrame angry_left_bottom_cut[] = {
    {0.00f, 0.05f, EasingType::LINEAR}, {0.25f, 0.15f, EasingType::EASE_OUT_CUBIC},
    {0.75f, 0.13f}, {1.00f, 0.05f, EasingType::EASE_IN_CUBIC}
};
static KeyFrame angry_left_rotation_deg[] = {
    {0.00f, 0.0f, EasingType::LINEAR}, {0.25f, -8.0f, EasingType::EASE_OUT_CUBIC},
    {0.75f, -7.0f}, {1.00f, 0.0f, EasingType::EASE_IN_CUBIC}
};
static KeyFrame angry_left_center_x[] = {
    {0.00f, 0.0f, EasingType::LINEAR}, {0.25f, -0.6f, EasingType::EASE_OUT_CUBIC},
    {0.75f, -0.53f}, {1.00f, 0.0f, EasingType::EASE_IN_CUBIC}
};
static KeyFrame angry_left_scale_y[] = {
    {0.00f, 0.85f, EasingType::LINEAR}, {0.25f, 0.65f, EasingType::EASE_OUT_CUBIC},
    {0.75f, 0.67f}, {1.00f, 0.85f, EasingType::EASE_IN_CUBIC}
};
static KeyFrame angry_left_scale_x[] = {
    {0.00f, 0.95f, EasingType::LINEAR}, {0.25f, 1.12f, EasingType::EASE_OUT_CUBIC},
    {0.75f, 1.10f}, {1.00f, 0.95f, EasingType::EASE_IN_CUBIC}
};

static KeyFrame angry_right_inner_corner_raise[] = {
    {0.00f, 0.0f, EasingType::LINEAR}, {0.32f, -0.37f, EasingType::EASE_OUT_CUBIC},
    {0.82f, -0.35f}, {1.00f, 0.0f, EasingType::EASE_IN_CUBIC}
};
static KeyFrame angry_right_outer_corner_raise[] = {
    {0.00f, 0.0f, EasingType::LINEAR}, {0.32f, 0.17f, EasingType::EASE_OUT_CUBIC},
    {0.82f, 0.15f}, {1.00f, 0.0f, EasingType::EASE_IN_CUBIC}
};
static KeyFrame angry_right_top_cut[] = {
    {0.00f, 0.10f, EasingType::LINEAR}, {0.32f, 0.28f, EasingType::EASE_OUT_CUBIC},
    {0.82f, 0.26f}, {1.00f, 0.10f, EasingType::EASE_IN_CUBIC}
};
static KeyFrame angry_right_bottom_cut[] = {
    {0.00f, 0.05f, EasingType::LINEAR}, {0.32f, 0.13f, EasingType::EASE_OUT_CUBIC},
    {0.82f, 0.11f}, {1.00f, 0.05f, EasingType::EASE_IN_CUBIC}
};
static KeyFrame angry_right_rotation_deg[] = {
    {0.00f, 0.0f, EasingType::LINEAR}, {0.32f, 8.0f, EasingType::EASE_OUT_CUBIC},
    {0.82f, 7.0f}, {1.00f, 0.0f, EasingType::EASE_IN_CUBIC}
};
static KeyFrame angry_right_center_x[] = {
    {0.00f, 0.0f, EasingType::LINEAR}, {0.32f, 0.6f, EasingType::EASE_OUT_CUBIC},
    {0.82f, 0.53f}, {1.00f, 0.0f, EasingType::EASE_IN_CUBIC}
};
static KeyFrame angry_right_scale_y[] = {
    {0.00f, 0.85f, EasingType::LINEAR}, {0.32f, 0.67f, EasingType::EASE_OUT_CUBIC},
    {0.82f, 0.69f}, {1.00f, 0.85f, EasingType::EASE_IN_CUBIC}
};
static KeyFrame angry_right_scale_x[] = {
    {0.00f, 0.95f, EasingType::LINEAR}, {0.32f, 1.14f, EasingType::EASE_OUT_CUBIC},
    {0.82f, 1.12f}, {1.00f, 0.95f, EasingType::EASE_IN_CUBIC}
};

// --- Happy hop keyframes (7 tracks) ---
static KeyFrame happy_face_offset_y[] = {
    {0.00f, 0.0f}, {0.10f, 0.10f, EasingType::EASE_IN_CUBIC},
    {0.20f, -0.50f, EasingType::EASE_OUT_CUBIC}, {0.45f, -0.50f},
    {0.55f, 0.18f, EasingType::EASE_IN_CUBIC}, {0.60f, 0.18f},
    {0.70f, -0.05f, EasingType::EASE_OUT_BACK}, {0.90f, 0.0f}
};
static KeyFrame happy_face_scale[] = {
    {0.00f, 1.0f}, {0.10f, 0.88f}, {0.20f, 1.10f},
    {0.45f, 1.10f}, {0.55f, 0.85f}, {0.70f, 1.04f},
    {0.90f, 1.0f}
};
static KeyFrame happy_eye_gap[] = {
    {0.00f, 1.0f}, {0.55f, 0.80f}, {0.70f, 1.15f, EasingType::EASE_OUT_BACK}, {0.90f, 1.0f}
};
static KeyFrame happy_left_outer_corner_raise[] = {
    {0.00f, 0.0f}, {0.25f, 0.35f, EasingType::EASE_OUT_CUBIC}, {0.55f, 0.30f}, {0.90f, 0.0f}
};
static KeyFrame happy_right_outer_corner_raise[] = {
    {0.00f, 0.0f}, {0.27f, 0.35f, EasingType::EASE_OUT_CUBIC}, {0.57f, 0.30f}, {0.90f, 0.0f}
};
static KeyFrame happy_left_top_cut[] = {
    {0.00f, 0.10f}, {0.25f, 0.40f, EasingType::EASE_OUT_CUBIC}, {0.55f, 0.35f}, {0.90f, 0.10f}
};
static KeyFrame happy_right_top_cut[] = {
    {0.00f, 0.10f}, {0.27f, 0.40f, EasingType::EASE_OUT_CUBIC}, {0.57f, 0.35f}, {0.90f, 0.10f}
};

// --- Double take soft keyframes (5 tracks) ---
static KeyFrame dt_left_center_x[] = {
    {0.00f, 0.0f}, {0.25f, 0.90f, EasingType::EASE_OUT_CUBIC},
    {0.65f, 0.0f, EasingType::EASE_IN_OUT_SINE},
    {0.90f, 0.70f, EasingType::EASE_OUT_SINE},
    {1.30f, 0.56f}, {1.80f, 0.0f, EasingType::EASE_IN_OUT_SINE}
};
static KeyFrame dt_right_center_x[] = {
    {0.00f, 0.0f}, {0.27f, 0.84f, EasingType::EASE_OUT_CUBIC},
    {0.67f, 0.0f, EasingType::EASE_IN_OUT_SINE},
    {0.92f, 0.64f, EasingType::EASE_OUT_SINE},
    {1.32f, 0.50f}, {1.80f, 0.0f, EasingType::EASE_IN_OUT_SINE}
};
static KeyFrame dt_face_offset_x[] = {
    {0.00f, 0.0f}, {0.25f, 0.40f, EasingType::EASE_OUT_CUBIC},
    {0.65f, 0.0f, EasingType::EASE_IN_CUBIC},
    {0.90f, 0.30f, EasingType::EASE_OUT_CUBIC},
    {1.30f, 0.24f}, {1.80f, 0.0f, EasingType::EASE_IN_CUBIC}
};
static KeyFrame dt_left_top_cut[] = {
    {0.00f, 0.10f}, {0.25f, 0.08f}, {0.65f, 0.10f},
    {0.90f, 0.08f}, {1.30f, 0.08f}, {1.80f, 0.10f}
};
static KeyFrame dt_right_top_cut[] = {
    {0.00f, 0.10f}, {0.27f, 0.09f}, {0.67f, 0.10f},
    {0.92f, 0.09f}, {1.32f, 0.09f}, {1.80f, 0.10f}
};

// --- Breathing keyframes (1 track) ---
static KeyFrame breath_offset_y[] = {
    {0.00f, 0.0f, EasingType::LINEAR},
    {1.00f, 0.08f, EasingType::EASE_IN_OUT_SINE},
    {2.00f, 0.0f, EasingType::EASE_IN_OUT_SINE}
};

// --- Micro tilt keyframes (1 track) ---
static KeyFrame tilt[] = {
    {0.00f, 0.0f, EasingType::LINEAR},
    {1.00f, 5.0f, EasingType::EASE_IN_OUT_SINE},
    {2.00f, 0.0f, EasingType::EASE_IN_OUT_SINE}
};

// --- Slowblink keyframes (4 tracks, 3 keyframes each) ---
static KeyFrame sb_left_top[] = {
    {0.00f, 0.0f, EasingType::LINEAR},
    {0.50f, 0.5f, EasingType::EASE_IN_OUT_SINE},
    {1.00f, 0.0f, EasingType::EASE_IN_OUT_SINE}
};
static KeyFrame sb_left_bottom[] = {
    {0.00f, 0.0f, EasingType::LINEAR},
    {0.50f, 0.5f, EasingType::EASE_IN_OUT_SINE},
    {1.00f, 0.0f, EasingType::EASE_IN_OUT_SINE}
};
static KeyFrame sb_right_top[] = {
    {0.00f, 0.0f, EasingType::LINEAR},
    {0.50f, 0.5f, EasingType::EASE_IN_OUT_SINE},
    {1.00f, 0.0f, EasingType::EASE_IN_OUT_SINE}
};
static KeyFrame sb_right_bottom[] = {
    {0.00f, 0.0f, EasingType::LINEAR},
    {0.50f, 0.5f, EasingType::EASE_IN_OUT_SINE},
    {1.00f, 0.0f, EasingType::EASE_IN_OUT_SINE}
};

// --- Doubleblink keyframes (4 tracks, 5 keyframes each) ---
static KeyFrame db_left_top[] = {
    {0.00f, 0.0f, EasingType::LINEAR}, {0.10f, 0.5f, EasingType::LINEAR},
    {0.20f, 0.0f, EasingType::LINEAR}, {0.30f, 0.5f, EasingType::LINEAR},
    {0.40f, 0.0f, EasingType::LINEAR}
};
static KeyFrame db_left_bottom[] = {
    {0.00f, 0.0f, EasingType::LINEAR}, {0.10f, 0.5f, EasingType::LINEAR},
    {0.20f, 0.0f, EasingType::LINEAR}, {0.30f, 0.5f, EasingType::LINEAR},
    {0.40f, 0.0f, EasingType::LINEAR}
};
static KeyFrame db_right_top[] = {
    {0.00f, 0.0f, EasingType::LINEAR}, {0.10f, 0.5f, EasingType::LINEAR},
    {0.20f, 0.0f, EasingType::LINEAR}, {0.30f, 0.5f, EasingType::LINEAR},
    {0.40f, 0.0f, EasingType::LINEAR}
};
static KeyFrame db_right_bottom[] = {
    {0.00f, 0.0f, EasingType::LINEAR}, {0.10f, 0.5f, EasingType::LINEAR},
    {0.20f, 0.0f, EasingType::LINEAR}, {0.30f, 0.5f, EasingType::LINEAR},
    {0.40f, 0.0f, EasingType::LINEAR}
};

// --- Sleepidle keyframes (1 track) ---
static KeyFrame sleep_offset_y[] = {
    {0.00f, 0.0f, EasingType::LINEAR},
    {2.00f, 0.05f, EasingType::EASE_IN_OUT_SINE},
    {4.00f, 0.0f, EasingType::EASE_IN_OUT_SINE}
};

// --- Test horizontal keyframes (2 tracks) ---
static KeyFrame test_lx[] = {
    {0.00f, 0.0f, EasingType::LINEAR},
    {0.25f, -1.0f, EasingType::EASE_OUT_CUBIC},
    {0.50f, 0.0f, EasingType::EASE_IN_OUT_SINE},
    {0.75f, 1.0f, EasingType::EASE_OUT_CUBIC},
    {1.00f, 0.0f, EasingType::EASE_IN_OUT_SINE}
};
static KeyFrame test_rx[] = {
    {0.00f, 0.0f, EasingType::LINEAR},
    {0.25f, -1.0f, EasingType::EASE_OUT_CUBIC},
    {0.50f, 0.0f, EasingType::EASE_IN_OUT_SINE},
    {0.75f, 1.0f, EasingType::EASE_OUT_CUBIC},
    {1.00f, 0.0f, EasingType::EASE_IN_OUT_SINE}
};

// --- Timeline Clip objects ---
static Clip clip_angry = {"angry", 1.0f, 12, true, true, nullptr, 0, 0, &timeline_angry};
static Clip clip_happy_hop = {"happy_hop", 0.9f, 3, false, true, nullptr, 0, 0, &timeline_happy_hop};
static Clip clip_double_take_soft = {"double_take_soft", 1.8f, 3, false, true, nullptr, 0, 0, &timeline_double_take_soft};
static Clip clip_breathing = {"breathing", 2.0f, 1, true, true, nullptr, 0, 0, &timeline_breathing};
static Clip clip_micro_tilt = {"micro_tilt", 2.0f, 1, true, true, nullptr, 0, 0, &timeline_micro_tilt};
static Clip clip_slowblink = {"slowblink", 1.0f, 5, false, true, nullptr, 0, 0, &timeline_slowblink};
static Clip clip_doubleblink = {"doubleblink", 0.4f, 5, false, true, nullptr, 0, 0, &timeline_doubleblink};
static Clip clip_sleepidle = {"sleepidle", 4.0f, 1, true, true, nullptr, 0, 0, &timeline_sleepidle};
static Clip clip_test_horizontal = {"test_horizontal", 2.0f, 3, false, false, nullptr, 0, 0, &timeline_test_horizontal};

// ==================== Initialization ====================

static void Init() {
    // Populate simple keyframe arrays (same as original implementation)
    blink_kf[0]={0.00f,0.0f,EasingType::LINEAR}; blink_kf[1]={0.12f,0.5f,EasingType::EASE_IN_QUAD}; blink_kf[2]={0.25f,0.0f,EasingType::EASE_OUT_QUAD};
    blink_kf[3]={0.00f,0.0f,EasingType::LINEAR}; blink_kf[4]={0.12f,0.5f,EasingType::EASE_IN_QUAD}; blink_kf[5]={0.25f,0.0f,EasingType::EASE_OUT_QUAD};
    boot_kf[0]={0.0f,0.5f,EasingType::EASE_OUT_QUAD}; boot_kf[1]={0.8f,0.05f,EasingType::EASE_OUT_BOUNCE};
    boot_kf[2]={0.0f,0.5f,EasingType::EASE_OUT_QUAD}; boot_kf[3]={0.8f,0.05f,EasingType::EASE_OUT_BOUNCE};
    happy_kf[0]={0.0f,0.1f,EasingType::LINEAR}; happy_kf[1]={0.25f,0.55f,EasingType::EASE_OUT_QUAD}; happy_kf[2]={0.5f,0.1f,EasingType::EASE_IN_QUAD};
    happy_kf[3]={0.0f,0.1f,EasingType::LINEAR}; happy_kf[4]={0.25f,0.55f,EasingType::EASE_OUT_QUAD}; happy_kf[5]={0.5f,0.1f,EasingType::EASE_IN_QUAD};
    sad_kf[0]={0.0f,0.0f,EasingType::LINEAR}; sad_kf[1]={0.3f,-0.3f,EasingType::EASE_OUT_QUAD}; sad_kf[2]={0.6f,0.0f,EasingType::EASE_IN_QUAD};
    sad_kf[3]={0.0f,0.0f,EasingType::LINEAR}; sad_kf[4]={0.3f,-0.3f,EasingType::EASE_OUT_QUAD}; sad_kf[5]={0.6f,0.0f,EasingType::EASE_IN_QUAD};
    surprised_kf[0]={0.0f,0.0f,EasingType::LINEAR}; surprised_kf[1]={0.4f,1.0f,EasingType::EASE_OUT_QUAD};
    surprised_kf[2]={0.0f,0.0f,EasingType::LINEAR}; surprised_kf[3]={0.4f,1.0f,EasingType::EASE_OUT_QUAD};
    thinking_kf[0]={0.0f,0.0f,EasingType::LINEAR}; thinking_kf[1]={0.5f,0.3f,EasingType::EASE_IN_OUT_QUAD};
    thinking_kf[2]={0.0f,0.0f,EasingType::LINEAR}; thinking_kf[3]={0.5f,0.3f,EasingType::EASE_IN_OUT_QUAD};
    sleep_kf[0]={0.0f,0.0f,EasingType::LINEAR}; sleep_kf[1]={0.5f,0.5f,EasingType::EASE_IN_QUAD}; sleep_kf[2]={1.0f,0.5f,EasingType::LINEAR};
    sleep_kf[3]={0.0f,0.0f,EasingType::LINEAR}; sleep_kf[4]={0.5f,0.5f,EasingType::EASE_IN_QUAD}; sleep_kf[5]={1.0f,0.5f,EasingType::LINEAR};
    wake_kf[0]={0.0f,0.5f,EasingType::LINEAR}; wake_kf[1]={0.5f,0.0f,EasingType::EASE_OUT_QUAD}; wake_kf[2]={0.8f,0.0f,EasingType::LINEAR};
    wake_kf[3]={0.0f,0.5f,EasingType::LINEAR}; wake_kf[4]={0.5f,0.0f,EasingType::EASE_OUT_QUAD}; wake_kf[5]={0.8f,0.0f,EasingType::LINEAR};
}

static void InitTimelines() {
    if (timelines_initialized) return;
    timelines_initialized = true;

    // Angry: 16 tracks
    timeline_angry.AddTrack("left.inner_corner_raise", angry_left_inner_corner_raise, 4);
    timeline_angry.AddTrack("left.outer_corner_raise", angry_left_outer_corner_raise, 4);
    timeline_angry.AddTrack("left.top_cut", angry_left_top_cut, 4);
    timeline_angry.AddTrack("left.bottom_cut", angry_left_bottom_cut, 4);
    timeline_angry.AddTrack("left.rotation_deg", angry_left_rotation_deg, 4);
    timeline_angry.AddTrack("left.center_x", angry_left_center_x, 4);
    timeline_angry.AddTrack("left.scale_y", angry_left_scale_y, 4);
    timeline_angry.AddTrack("left.scale_x", angry_left_scale_x, 4);
    timeline_angry.AddTrack("right.inner_corner_raise", angry_right_inner_corner_raise, 4);
    timeline_angry.AddTrack("right.outer_corner_raise", angry_right_outer_corner_raise, 4);
    timeline_angry.AddTrack("right.top_cut", angry_right_top_cut, 4);
    timeline_angry.AddTrack("right.bottom_cut", angry_right_bottom_cut, 4);
    timeline_angry.AddTrack("right.rotation_deg", angry_right_rotation_deg, 4);
    timeline_angry.AddTrack("right.center_x", angry_right_center_x, 4);
    timeline_angry.AddTrack("right.scale_y", angry_right_scale_y, 4);
    timeline_angry.AddTrack("right.scale_x", angry_right_scale_x, 4);

    // Happy hop: 7 tracks
    timeline_happy_hop.AddTrack("face.face_offset_y", happy_face_offset_y, 8);
    timeline_happy_hop.AddTrack("face.face_scale", happy_face_scale, 7);
    timeline_happy_hop.AddTrack("face.eye_gap", happy_eye_gap, 4);
    timeline_happy_hop.AddTrack("left.outer_corner_raise", happy_left_outer_corner_raise, 4);
    timeline_happy_hop.AddTrack("right.outer_corner_raise", happy_right_outer_corner_raise, 4);
    timeline_happy_hop.AddTrack("left.top_cut", happy_left_top_cut, 4);
    timeline_happy_hop.AddTrack("right.top_cut", happy_right_top_cut, 4);

    // Double take soft: 5 tracks
    timeline_double_take_soft.AddTrack("left.center_x", dt_left_center_x, 6);
    timeline_double_take_soft.AddTrack("right.center_x", dt_right_center_x, 6);
    timeline_double_take_soft.AddTrack("face.face_offset_x", dt_face_offset_x, 6);
    timeline_double_take_soft.AddTrack("left.top_cut", dt_left_top_cut, 6);
    timeline_double_take_soft.AddTrack("right.top_cut", dt_right_top_cut, 6);

    // Breathing: 1 track
    timeline_breathing.AddTrack("face.face_offset_y", breath_offset_y, 3);

    // Micro tilt: 1 track
    timeline_micro_tilt.AddTrack("face.face_tilt", tilt, 3);

    // Slowblink: 4 tracks
    timeline_slowblink.AddTrack("left.top_cut", sb_left_top, 3);
    timeline_slowblink.AddTrack("left.bottom_cut", sb_left_bottom, 3);
    timeline_slowblink.AddTrack("right.top_cut", sb_right_top, 3);
    timeline_slowblink.AddTrack("right.bottom_cut", sb_right_bottom, 3);

    // Doubleblink: 4 tracks
    timeline_doubleblink.AddTrack("left.top_cut", db_left_top, 5);
    timeline_doubleblink.AddTrack("left.bottom_cut", db_left_bottom, 5);
    timeline_doubleblink.AddTrack("right.top_cut", db_right_top, 5);
    timeline_doubleblink.AddTrack("right.bottom_cut", db_right_bottom, 5);

    // Sleepidle: 1 track
    timeline_sleepidle.AddTrack("face.face_offset_y", sleep_offset_y, 3);

    // Test horizontal: 2 tracks
    timeline_test_horizontal.AddTrack("left.center_x",  test_lx, 5);
    timeline_test_horizontal.AddTrack("right.center_x", test_rx, 5);
}

// ==================== Public API ====================

const Clip* AnimationLibrary::Get(const char* n) {
    Init(); // ensure simple clips are initialized
    if (!strcmp(n, "blink")) return &clip_blink;
    if (!strcmp(n, "boot_open")) return &clip_boot;
    if (!strcmp(n, "happy")) return &clip_happy;
    if (!strcmp(n, "sad")) return &clip_sad;
    if (!strcmp(n, "surprised")) return &clip_surprised;
    if (!strcmp(n, "thinking")) return &clip_thinking;
    if (!strcmp(n, "sleep_enter")) return &clip_sleep;
    if (!strcmp(n, "wake_enter")) return &clip_wake;
    // Timeline-based clips
    if (!strcmp(n, "angry")) { InitTimelines(); return &clip_angry; }
    if (!strcmp(n, "happy_hop")) { InitTimelines(); return &clip_happy_hop; }
    if (!strcmp(n, "double_take_soft")) { InitTimelines(); return &clip_double_take_soft; }
    if (!strcmp(n, "breathing")) { InitTimelines(); return &clip_breathing; }
    if (!strcmp(n, "micro_tilt")) { InitTimelines(); return &clip_micro_tilt; }
    if (!strcmp(n, "slowblink")) { InitTimelines(); return &clip_slowblink; }
    if (!strcmp(n, "doubleblink")) { InitTimelines(); return &clip_doubleblink; }
    if (!strcmp(n, "sleepidle")) { InitTimelines(); return &clip_sleepidle; }
    if (!strcmp(n, "test_horizontal")) { InitTimelines(); return &clip_test_horizontal; }
    return nullptr;
}
const Clip* AnimationLibrary::Happy()      { Init(); return &clip_happy; }
const Clip* AnimationLibrary::Sad()        { Init(); return &clip_sad; }
const Clip* AnimationLibrary::Angry()      { InitTimelines(); return &clip_angry; }
const Clip* AnimationLibrary::Surprised()  { Init(); return &clip_surprised; }
const Clip* AnimationLibrary::Thinking()   { Init(); return &clip_thinking; }
const Clip* AnimationLibrary::SleepEnter() { Init(); return &clip_sleep; }
const Clip* AnimationLibrary::WakeEnter()  { Init(); return &clip_wake; }
const Clip* AnimationLibrary::Blink()      { Init(); return &clip_blink; }
const Clip* AnimationLibrary::BootOpen()   { Init(); return &clip_boot; }
const Clip* AnimationLibrary::Breathing()  { InitTimelines(); return &clip_breathing; }
const Clip* AnimationLibrary::MicroTilt()  { InitTimelines(); return &clip_micro_tilt; }
const Clip* AnimationLibrary::SlowBlink()  { InitTimelines(); return &clip_slowblink; }
const Clip* AnimationLibrary::DoubleBlink() { InitTimelines(); return &clip_doubleblink; }
const Clip* AnimationLibrary::SleepIdle()  { InitTimelines(); return &clip_sleepidle; }
const Clip* AnimationLibrary::TestHorizontal() { InitTimelines(); return &clip_test_horizontal; }

} // namespace procedural
