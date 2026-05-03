// procedural/eye_shape.cc
#include "eye_shape.h"
#include <cmath>

namespace procedural {

EyeShape::Polygon EyeShape::GenerateContour(const EyeParameters& eye, int16_t cx, int16_t cy, int16_t base_w, int16_t base_h) {
    Polygon poly; poly.count = 0;
    if (!eye.visible) return poly;
    
    float rx = base_w * 0.5f * eye.scale_x;
    float ry = base_h * 0.5f * eye.scale_y;
    if (rx < 2.0f) rx = 2.0f;
    if (ry < 2.0f) ry = 2.0f;
    
    Point temp[16];
    for (uint8_t i = 0; i < 16; ++i) {
        float angle = (float(i) / 16.0f) * 2.0f * 3.14159f;
        float x = cosf(angle) * rx;
        float y = sinf(angle) * ry;
        float ca = cosf(angle);
        if (ca > 0.0f) y += eye.inner_corner_raise * 8.0f;
        else y += eye.outer_corner_raise * 8.0f;
        if (eye.top_cut > 0.0f) {
            float tl = -ry * (1.0f - eye.top_cut);
            if (y < tl) y = tl;
        }
        if (eye.bottom_cut > 0.0f) {
            float bl = ry * (1.0f - eye.bottom_cut);
            if (y > bl) y = bl;
        }
        if (eye.top_cut + eye.bottom_cut >= 1.0f) { poly.count = 0; return poly; }
        temp[i] = { int16_t(x), int16_t(y) };
    }
    
    float rad = eye.rotation_deg * 3.14159f / 180.0f;
    float cos_r = cosf(rad), sin_r = sinf(rad);
    for (uint8_t i = 0; i < 16; ++i) {
        float xr = temp[i].x * cos_r - temp[i].y * sin_r;
        float yr = temp[i].x * sin_r + temp[i].y * cos_r;
        float px = cx + eye.center_x * kCenterPxPerUnit + xr;
        float py = cy + eye.center_y * kCenterPxPerUnit + yr;
        Point p = { int16_t(px), int16_t(py) };
        if (poly.count > 0 && p.x == poly.points[poly.count-1].x && p.y == poly.points[poly.count-1].y) continue;
        if (poly.count < MAX_POINTS) poly.points[poly.count++] = p;
    }

    // Ensure we have at least 3 valid points for rendering
    if (poly.count < 3) poly.count = 0;

    return poly;
}

Rect EyeShape::GetBounds(const Polygon& poly) {
    if (poly.count == 0) return {0,0,0,0};
    int16_t mx = poly.points[0].x, my = poly.points[0].y, Mx = mx, My = my;
    for (uint8_t i = 1; i < poly.count; ++i) {
        if (poly.points[i].x < mx) mx = poly.points[i].x;
        if (poly.points[i].x > Mx) Mx = poly.points[i].x;
        if (poly.points[i].y < my) my = poly.points[i].y;
        if (poly.points[i].y > My) My = poly.points[i].y;
    }
    return { mx, my, int16_t(Mx-mx+1), int16_t(My-my+1) };
}

EyeParameters EyeShape::PresetNeutral() { EyeParameters e; e.scale_x=0.90f; e.scale_y=1.00f; e.roundness=0.7f; e.top_cut=0.0f; e.bottom_cut=0.0f; e.brightness=1.0f; return e; }
EyeParameters EyeShape::PresetBlinkClosed() { EyeParameters e; e.scale_x=0.95f; e.scale_y=0.85f; e.top_cut=0.55f; e.bottom_cut=0.5f; e.brightness=0.3f; return e; }
EyeParameters EyeShape::PresetHappy()  { EyeParameters e; e.scale_x=1.0f; e.scale_y=1.0f; e.inner_corner_raise=0.25f; e.outer_corner_raise=0.35f; e.top_cut=0.15f; e.bottom_cut=0.0f; e.roundness=0.8f; return e; }
EyeParameters EyeShape::PresetAngry()  { EyeParameters e; e.scale_x=0.95f; e.scale_y=1.0f; e.inner_corner_raise=-0.4f; e.outer_corner_raise=0.2f; e.rotation_deg=-6.0f; e.top_cut=0.25f; e.bottom_cut=0.2f; e.roundness=0.3f; return e; }
EyeParameters EyeShape::PresetSad()    { EyeParameters e; e.scale_x=0.9f; e.scale_y=1.0f; e.inner_corner_raise=0.1f; e.outer_corner_raise=-0.3f; e.top_cut=0.15f; e.bottom_cut=0.2f; e.roundness=0.9f; return e; }
EyeParameters EyeShape::PresetSurprised() { EyeParameters e; e.scale_x=1.1f; e.scale_y=1.0f; e.roundness=0.9f; e.brightness=1.0f; return e; }
EyeParameters EyeShape::PresetSleepy() { EyeParameters e; e.scale_x=0.9f; e.scale_y=0.95f; e.top_cut=0.20f; e.bottom_cut=0.25f; e.roundness=0.8f; return e; }
EyeParameters EyeShape::PresetFocused() { EyeParameters e; e.scale_x=0.90f; e.scale_y=1.00f; e.top_cut=0.15f; e.bottom_cut=0.1f; e.roundness=0.5f; return e; }

} // namespace procedural
