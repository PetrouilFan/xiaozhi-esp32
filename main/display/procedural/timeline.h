#pragma once
#include "types.h"
#include "easing.h"

namespace procedural {

class Timeline {
public:
    static constexpr uint8_t MAX_TRACKS = 8, MAX_KEYFRAMES = 6;
    struct Track {
        const char* name;
        KeyFrame keyframes[MAX_KEYFRAMES];
        uint8_t count;
    };
    bool AddTrack(const char* name, const KeyFrame* frames, uint8_t count);
    float Evaluate(uint8_t track_idx, float time) const;
    void ApplyToState(FaceState& state, float time) const;
    float duration() const { return duration_; }
private:
    Track tracks_[MAX_TRACKS];
    uint8_t track_count_ = 0;
    float duration_ = 0.0f;
    static void ApplyValue(FaceState& s, const char* name, float v);
};

} // namespace procedural