// procedural/scheduler.cc
#include "scheduler.h"
#include "timeline.h"
#include <cstring>
#include <string>

namespace procedural {

void Scheduler::Play(const Clip* c) {
    if (!c || active_count_ >= MAX_ACTIVE) return;
    for (uint8_t i = 0; i < active_count_; ++i) { if (active_[i].clip == c) return; }
    active_[active_count_].clip = c;
    active_[active_count_].start = 0.0f;
    active_[active_count_++];
}

void Scheduler::Stop(const char* n) {
    size_t l = strlen(n);
    for (uint8_t i = 0; i < active_count_; ) {
        if (strncmp(active_[i].clip->name, n, l) == 0) Remove(i); else ++i;
    }
}

FaceState Scheduler::Update(float t) {
    FaceState s = base_state_;
    for (uint8_t i = 0; i < active_count_; ) {
        Active& a = active_[i];
        if (a.start == 0.0f) a.start = t;
        a.local = t - a.start;
        if (!a.clip->loop && a.local >= a.clip->duration) { Remove(i); continue; }
        if (a.clip->loop && a.clip->duration > 0.0f) a.local = fmodf(a.local, a.clip->duration);

        float p = a.clip->duration > 0.0f ? a.local / a.clip->duration : 1.0f;
        if (p > 1.0f) p = 1.0f;
        const char* n = a.clip->name;

        if (!strcmp(n, "blink")) {
            float cut = p < 0.5f ? p*2.0f*0.5f : (1.0f-p)*2.0f*0.5f;
            s.left.top_cut = s.left.bottom_cut = s.right.top_cut = s.right.bottom_cut = cut;
        } else if (!strcmp(n, "boot_open")) {
            float open = p*p*(3.0f-2.0f*p);
            float cut = 0.5f*(1.0f-open);
            s.left.top_cut = s.left.bottom_cut = s.right.top_cut = s.right.bottom_cut = cut;
            s.left.brightness = s.right.brightness = 0.3f + 0.7f*open;
        } else if (!strcmp(n, "happy")) {
            float sq = sinf(p*3.14159f);
            s.left.scale_y = s.right.scale_y = 0.85f - sq*0.3f;
            s.left.top_cut = s.right.top_cut = sq*0.3f;
        } else if (!strcmp(n, "sad")) {
            float d = sinf(p*3.14159f);
            s.left.outer_corner_raise = s.right.outer_corner_raise = -0.3f*d;
            s.left.top_cut = s.right.top_cut = 0.15f*d;
        } else if (!strcmp(n, "surprised")) {
            float w = sinf(p*3.14159f);
            s.left.scale_x = s.right.scale_x = 1.0f + w*0.2f;
            s.left.scale_y = s.right.scale_y = 1.0f + w*0.3f;
        } else if (!strcmp(n, "thinking")) {
            float sq = sinf(p*3.14159f);
            s.left.scale_y = s.right.scale_y = 0.85f - sq*0.15f;
        } else if (!strcmp(n, "sleep_enter")) {
            s.left.top_cut = s.right.top_cut = p*0.5f;
            s.left.bottom_cut = s.right.bottom_cut = p*0.4f;
            s.left.brightness = s.right.brightness = 1.0f - p*0.7f;
        } else if (!strcmp(n, "wake_enter")) {
            float open = 1.0f-p;
            s.left.top_cut = s.right.top_cut = open*0.5f;
            s.left.bottom_cut = s.right.bottom_cut = open*0.4f;
            s.left.brightness = s.right.brightness = 0.3f + p*0.7f;
        } else if (a.clip->timeline) {
            a.clip->timeline->ApplyToState(s, a.local);
        }
        ++i;
    }
    return s;
}

bool Scheduler::IsPlaying(const char* n) const {
    for (uint8_t i = 0; i < active_count_; ++i) { if (!strcmp(active_[i].clip->name, n)) return true; }
    return false;
}

void Scheduler::Remove(uint8_t idx) {
    if (idx >= active_count_) return;
    for (uint8_t i = idx; i < active_count_-1; ++i) active_[i] = active_[i+1];
    --active_count_;
}

} // namespace procedural
