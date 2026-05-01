#pragma once
#include "types.h"

namespace procedural {

class Scheduler {
public:
    static constexpr uint8_t MAX_ACTIVE = 4;
    struct Active { const Clip* clip; float start; float local; };

    Scheduler() : active_count_(0) {}
    void SetBaseState(const FaceState& s) { base_state_ = s; }
    void Play(const Clip* clip);
    void Stop(const char* name);
    FaceState Update(float t);
    bool IsPlaying(const char* name) const;
    const FaceState& base() const { return base_state_; }

private:
    FaceState base_state_;
    Active active_[MAX_ACTIVE];
    uint8_t active_count_;
    void Remove(uint8_t i);
};

} // namespace procedural
