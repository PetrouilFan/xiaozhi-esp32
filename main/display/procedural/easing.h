#pragma once
#include "types.h"

namespace procedural {
inline float Interpolate(float a, float b, float t, EasingType e) {
    float c = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    switch (e) {
        case EasingType::LINEAR:          break;
        case EasingType::EASE_IN_QUAD:    c = c * c; break;
        case EasingType::EASE_OUT_QUAD:   c = 1.0f - (1.0f-c)*(1.0f-c); break;
        case EasingType::EASE_IN_OUT_QUAD:
            c = c < 0.5f ? 2.0f*c*c : 1.0f - powf(-2.0f*c+2.0f, 2)/2.0f;
            break;
        case EasingType::EASE_OUT_BOUNCE:
            if (c < 1.0f/2.75f) c = 7.5625f*c*c;
            else if (c < 2.0f/2.75f) { c -= 1.5f/2.75f; c = 7.5625f*c*c + 0.75f; }
            else if (c < 2.5f/2.75f) { c -= 2.25f/2.75f; c = 7.5625f*c*c + 0.9375f; }
            else { c -= 2.625f/2.75f; c = 7.5625f*c*c + 0.984375f; }
            break;
        case EasingType::EASE_IN_CUBIC:   c = c * c * c; break;
        case EasingType::EASE_OUT_CUBIC:  c = 1.0f - powf(1.0f - c, 3); break;
        case EasingType::EASE_IN_OUT_SINE: c = -(cosf(c*3.14159f) - 1.0f) / 2.0f; break;
        case EasingType::EASE_OUT_SINE:   c = sinf(c * 3.14159f / 2.0f); break;
        case EasingType::EASE_OUT_BACK:
            c = c * c * (3.0f - 2.0f * c);
            break;
        default: break;
    }
    return a + (b - a) * c;
}
} // namespace procedural