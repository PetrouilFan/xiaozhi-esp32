#pragma once
#include "types.h"

namespace procedural {

class EyeShape {
public:
    static constexpr uint8_t MAX_POINTS = 32;
    struct Polygon { Point points[MAX_POINTS]; uint8_t count; };
    static Polygon GenerateContour(const EyeParameters& eye, int16_t cx, int16_t cy, int16_t base_w=48, int16_t base_h=64);
    static Rect GetBounds(const Polygon& poly);
    static EyeParameters PresetNeutral();
    static EyeParameters PresetBlinkClosed();
    static EyeParameters PresetHappy();
    static EyeParameters PresetAngry();
    static EyeParameters PresetSad();
    static EyeParameters PresetSurprised();
    static EyeParameters PresetSleepy();
    static EyeParameters PresetFocused();
};

} // namespace procedural