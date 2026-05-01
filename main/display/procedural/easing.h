#pragma once
#include "types.h"

namespace procedural {
inline float Interpolate(float a, float b, float t, EasingType e) {
    float c = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    switch (e) {
        case EasingType::EASE_IN_QUAD:     c = c * c; break;
        case EasingType::EASE_OUT_QUAD:    c = 1.0f - (1.0f-c)*(1.0f-c); break;
        case EasingType::EASE_IN_OUT_QUAD: c = c < 0.5f ? 2.0f*c*c : 1.0f - powf(-2.0f*c+2.0f,2)/2.0f; break;
        default: break;
    }
    return a + (b - a) * c;
}
} // namespace procedural