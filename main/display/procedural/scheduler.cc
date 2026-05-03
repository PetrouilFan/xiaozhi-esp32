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
        } else if (!strcmp(n, "bootclose")) {
            float open = p*p*(3.0f-2.0f*p);
            float cut = 0.75f*(1.0f-open) + 0.10f*open;
            s.left.top_cut = s.left.bottom_cut = cut;
            s.right.top_cut = s.right.bottom_cut = cut;
            s.left.brightness = s.right.brightness = 0.3f + 0.7f*open;
            // Unhide eyes as they start opening
            s.left.visible = s.right.visible = true;
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
// Common tier (high frequency, ambient)
// Phase masks: IDLE=0x0002, LISTENING=0x0004, THINKING=0x0008, SPEAKING=0x0010, SLEEPING=0x0020
{"breathing", nullptr, BehaviorTier::COMMON, 0x000E, 2000, 8}, // IDLE|LISTENING|THINKING
{"micro_tilt", nullptr, BehaviorTier::COMMON, 0x000A, 2500, 7}, // IDLE|THINKING
{"microtiltswap", nullptr, BehaviorTier::COMMON, 0x0002, 3000, 5}, // IDLE
{"blink", nullptr, BehaviorTier::COMMON, 0x000E, 3000, 10}, // IDLE|LISTENING|THINKING
{"doubleblink", nullptr, BehaviorTier::COMMON, 0x0006, 6000, 4}, // IDLE|LISTENING
{"wander", nullptr, BehaviorTier::COMMON, 0x0006, 5000, 6}, // IDLE|LISTENING

// Occasional tier (medium frequency)
{"slowblink", nullptr, BehaviorTier::OCCASIONAL, 0x0006, 5000, 5}, // IDLE|LISTENING
{"sleepyrecover", nullptr, BehaviorTier::OCCASIONAL, 0x0002, 8000, 3}, // IDLE

// Rare tier (low frequency, expressive)
{"suspicious", nullptr, BehaviorTier::RARE, 0x0006, 8000, 3}, // IDLE|LISTENING
{"curiouspeek", nullptr, BehaviorTier::RARE, 0x0006, 10000, 2}, // IDLE|LISTENING
{"orbitsearch", nullptr, BehaviorTier::RARE, 0x0006, 7000, 3}, // IDLE|LISTENING
{"lookaway", nullptr, BehaviorTier::RARE, 0x000E, 6000, 3}, // IDLE|LISTENING|THINKING
{"yawn", nullptr, BehaviorTier::RARE, 0x0020, 15000, 2}, // SLEEPING only
{"sleeppeek", nullptr, BehaviorTier::RARE, 0x0020, 8000, 2}, // SLEEPING only

// Speaking tier (active during speech)
{"speaking_squish", nullptr, BehaviorTier::OCCASIONAL, 0x0010, 2000, 4}, // SPEAKING only
{"emphasis_blink", nullptr, BehaviorTier::OCCASIONAL, 0x0010, 3500, 3}, // SPEAKING only
};

static constexpr uint8_t kBehaviorCount = sizeof(behavior_pool) / sizeof(behavior_pool[0]);

} // anonymous namespace

bool Scheduler::TryPlayAutonomous(FacePhase phase, uint32_t now_ms) {
    // Phase-specific cooldown multipliers: speaking gets highest frequency
    uint16_t phase_multiplier = 1000;
    switch (phase) {
        case FacePhase::IDLE: phase_multiplier = 1000; break;       // baseline
        case FacePhase::LISTENING: phase_multiplier = 1500; break;  // less frequent - focus listening
        case FacePhase::THINKING: phase_multiplier = 1200; break;  // moderate - thinking
        case FacePhase::SPEAKING: phase_multiplier = 500; break;   // highest - active during speech
        case FacePhase::SLEEPING: phase_multiplier = 2000; break; // very low - sleeping
        default: phase_multiplier = 1000; break;
    }

    uint8_t tier_weights_common = 0, tier_weights_occasion = 0, tier_weights_rare = 0;

    for (uint8_t i = 0; i < kBehaviorCount; ++i) {
        const BehaviorEntry& b = behavior_pool[i];
        if (!(b.min_phase_mask & (1 << static_cast<uint8_t>(phase)))) continue;
        uint32_t effective_cooldown = (b.cooldown_ms * phase_multiplier) / 1000;
        if (now_ms - last_behavior_ms_[i] < effective_cooldown) continue;
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
            // Use name-based lookup via AnimationLibrary::Get
            const Clip* c = AnimationLibrary::Get(b.name);
            if (c) Play(c);
            last_behavior_ms_[i] = now_ms;
            return true;
        }
    }
    return false;
}

} // namespace procedural
