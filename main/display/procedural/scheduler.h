#pragma once
#include "types.h"
#include <cstring>

#ifdef CONFIG_PROCEDURAL_DEBUG
#include <esp_log.h>
#define SCHEDULER_DEBUG(fmt, ...) ESP_LOGI("Scheduler", fmt, ##__VA_ARGS__)
#else
#define SCHEDULER_DEBUG(fmt, ...)
#endif

namespace procedural {

enum class BehaviorTier { COMMON, OCCASIONAL, RARE };

struct BehaviorEntry {
    const char* name;
    const Clip* clip;
    BehaviorTier tier;
    uint8_t min_phase_mask;  // bitmask of FacePhase values allowed
    uint16_t cooldown_ms;
    uint8_t weight;  // relative probability within tier
};

class Scheduler {
public:
    static constexpr uint8_t MAX_ACTIVE = 6;
    static constexpr uint8_t MAX_BEHAVIORS = 32;
    struct Active { const Clip* clip; float start; float local; };

    Scheduler() : active_count_(0) {}
    void SetBaseState(const FaceState& s) { base_state_ = s; }
    void Play(const Clip* clip);
    void Play(const char* clip_name);
    void Stop(const char* name);
    FaceState Update(float t, FacePhase current_phase);
    bool IsPlaying(const char* name) const;
    const FaceState& base() const { return base_state_; }

    void SetPhase(FacePhase phase) { current_phase_ = phase; }
    bool TryPlayAutonomous(FacePhase phase, uint32_t now_ms);
    void ResetCooldowns() { memset(last_behavior_ms_, 0, sizeof(last_behavior_ms_)); }

private:
    FaceState base_state_;
    Active active_[MAX_ACTIVE];
    uint8_t active_count_;
    FacePhase current_phase_ = FacePhase::IDLE;
    uint32_t last_behavior_ms_[MAX_BEHAVIORS] = {0};

    void Remove(uint8_t i);
};

} // namespace procedural
