// procedural/timeline.cc
#include "timeline.h"
#include <cstring>

namespace procedural {

bool Timeline::AddTrack(const char* name, const KeyFrame* frames, uint8_t count) {
    if (track_count_ >= MAX_TRACKS || count == 0 || count > MAX_KEYFRAMES) return false;
    Track& t = tracks_[track_count_++];
    t.name = name; t.count = count;
    for (uint8_t i = 0; i < count; ++i) { t.keyframes[i] = frames[i]; }
    // Bubble sort by time
    for (uint8_t i = 0; i < count - 1; ++i) {
        for (uint8_t j = 0; j < count - i - 1; ++j) {
            if (t.keyframes[j].time > t.keyframes[j+1].time) {
                KeyFrame tmp = t.keyframes[j];
                t.keyframes[j] = t.keyframes[j+1];
                t.keyframes[j+1] = tmp;
            }
        }
    }
    return true;
}

float Timeline::Evaluate(uint8_t track_idx, float time) const {
    if (track_idx >= track_count_) return 0.0f;
    const Track& t = tracks_[track_idx];
    if (t.count == 0) return 0.0f;
    if (time <= t.keyframes[0].time) return t.keyframes[0].value;
    if (time >= t.keyframes[t.count-1].time) return t.keyframes[t.count-1].value;
    uint8_t idx = 1;
    for (; idx < t.count; ++idx) { if (time < t.keyframes[idx].time) break; }
    const KeyFrame& k0 = t.keyframes[idx-1];
    const KeyFrame& k1 = t.keyframes[idx];
    float dt = k1.time - k0.time;
    if (dt <= 0.0f) return k1.value;
    return Interpolate(k0.value, k1.value, (time - k0.time) / dt, k1.easing);
}

void Timeline::ApplyToState(FaceState& s, float time) const {
    for (uint8_t i = 0; i < track_count_; ++i) {
        ApplyValue(s, tracks_[i].name, Evaluate(i, time));
    }
}

static void set_eye(EyeParameters& e, const char* f, float v) {
    if      (!strcmp(f,"center_x"))           e.center_x=v;
    else if (!strcmp(f,"center_y"))           e.center_y=v;
    else if (!strcmp(f,"scale_x"))            e.scale_x=v;
    else if (!strcmp(f,"scale_y"))            e.scale_y=v;
    else if (!strcmp(f,"rotation_deg"))       e.rotation_deg=v;
    else if (!strcmp(f,"inner_corner_raise"))  e.inner_corner_raise=v;
    else if (!strcmp(f,"outer_corner_raise"))  e.outer_corner_raise=v;
    else if (!strcmp(f,"top_cut"))            e.top_cut=v;
    else if (!strcmp(f,"bottom_cut"))         e.bottom_cut=v;
    else if (!strcmp(f,"inner_tension"))      e.inner_tension=v;
    else if (!strcmp(f,"outer_tension"))      e.outer_tension=v;
    else if (!strcmp(f,"roundness"))          e.roundness=v;
    else if (!strcmp(f,"brightness"))         e.brightness=v;
    else if (!strcmp(f,"visible"))            e.visible=v>0.5f;
}

void Timeline::ApplyValue(FaceState& s, const char* name, float v) {
    const char* f = name;
    if (strncmp(name,"left.",5)==0) { set_eye(s.left, name+5, v); }
    else if (strncmp(name,"right.",6)==0) { set_eye(s.right, name+6, v); }
    else if (strncmp(name,"face.",5)==0) {
        f = name+5;
        if      (!strcmp(f,"warp"))   s.rig.face_warp=v;
        else if (!strcmp(f,"x"))      s.rig.face_x=v;
        else if (!strcmp(f,"y"))      s.rig.face_y=v;
        else if (!strcmp(f,"scale"))  s.rig.face_scale=v;
    }
}

} // namespace procedural
