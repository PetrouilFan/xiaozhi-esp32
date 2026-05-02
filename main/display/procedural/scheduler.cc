// procedural/scheduler.cc
#include "scheduler.h"
#include "animation_library.h"
#include "timeline.h"
#include <esp_log.h>
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

void Scheduler::Play(const char* clip_name) {
    if (!clip_name) return;
    const Clip* c = AnimationLibrary::Get(clip_name);
    if (c) Play(c);
}

void Scheduler::Stop(const char* n) {
    size_t l = strlen(n);
    for (uint8_t i = 0; i < active_count_; ) {
        if (strncmp(active_[i].clip->name, n, l) == 0) Remove(i); else ++i;
    }
}

FaceState Scheduler::Update(float t, FacePhase current_phase) {
    current_phase_ = current_phase;
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

namespace {

static const BehaviorEntry behavior_pool[] = {
    // Common tier (high frequency, idle-safe)
    {"breathing", nullptr, BehaviorTier::COMMON, 0x01, 3000, 10},      // IDLE only, 3s cooldown
    {"micro_tilt", nullptr, BehaviorTier::COMMON, 0x01, 2000, 8},
    {"microtiltswap", nullptr, BehaviorTier::COMMON, 0x01, 2500, 6},
    {"tinyfocusnarrow", nullptr, BehaviorTier::COMMON, 0x03, 4000, 5},   // IDLE|LISTENING

    // Occasional tier (medium frequency)
    {"softsquish", nullptr, BehaviorTier::OCCASIONAL, 0x01, 5000, 6},
    {"sleepyrecover", nullptr, BehaviorTier::OCCASIONAL, 0x01, 6000, 4},
    {"slowblink", nullptr, BehaviorTier::OCCASIONAL, 0x03, 3500, 8},
    {"doubleblink", nullptr, BehaviorTier::OCCASIONAL, 0x03, 3500, 5},

    // Rare tier (low frequency, expressive)
    {"suspicious", nullptr, BehaviorTier::RARE, 0x01, 8000, 3},
    {"curiouspeek", nullptr, BehaviorTier::RARE, 0x01, 10000, 2},
    {"yawn", nullptr, BehaviorTier::RARE, 0x10, 15000, 2},   // SLEEPING only
    {"orbitsearch", nullptr, BehaviorTier::RARE, 0x01, 7000, 3},
    {"sleeppeek", nullptr, BehaviorTier::RARE, 0x10, 8000, 2},   // SLEEPING only
};

static constexpr uint8_t kBehaviorCount = sizeof(behavior_pool) / sizeof(behavior_pool[0]);

} // anonymous namespace

bool Scheduler::TryPlayAutonomous(FacePhase phase, uint32_t now_ms) {
    uint8_t tier_weights_common = 0, tier_weights_occasion = 0, tier_weights_rare = 0;

    for (uint8_t i = 0; i < kBehaviorCount; ++i) {
        const BehaviorEntry& b = behavior_pool[i];
        if (!(b.min_phase_mask & (1 << static_cast<uint8_t>(phase)))) continue;
        if (now_ms - last_behavior_ms_[i] < b.cooldown_ms) continue;
        if (b.tier == BehaviorTier::COMMON) tier_weights_common += b.weight;
        else if (b.tier == BehaviorTier::OCCASIONAL) tier_weights_occasion += b.weight;
        else tier_weights_rare += b.weight;
    }

    uint32_t r = now_ms % 100;
    BehaviorTier selected_tier;
    if (r < 60) selected_tier = BehaviorTier::COMMON;
    else if (r < 85) selected_tier = BehaviorTier::OCCASIONAL;
    else selected_tier = BehaviorTier::RARE;

    uint8_t tier_total = 0;
    if (selected_tier == BehaviorTier::COMMON) tier_total = tier_weights_common;
    else if (selected_tier == BehaviorTier::OCCASIONAL) tier_total = tier_weights_occasion;
    else tier_total = tier_weights_rare;

    if (tier_total == 0) return false;

    uint8_t selection = (now_ms / 7) % tier_total;
    uint8_t cum = 0;
    for (uint8_t i = 0; i < kBehaviorCount; ++i) {
        const BehaviorEntry& b = behavior_pool[i];
        if (b.tier != selected_tier) continue;
        if (!(b.min_phase_mask & (1 << static_cast<uint8_t>(phase)))) continue;
        if (now_ms - last_behavior_ms_[i] < b.cooldown_ms) continue;

        cum += b.weight;
        if (cum > selection) {
            SCHEDULER_DEBUG("playing: %s, tier: %d", b.name, (int)selected_tier);
            if (b.clip) Play(b.clip);
            last_behavior_ms_[i] = now_ms;
            return true;
        }
    }
    return false;
}

} // namespace procedural
