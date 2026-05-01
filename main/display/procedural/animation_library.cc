// procedural/animation_library.cc
#include "animation_library.h"
#include <cstring>

namespace procedural {

static KeyFrame blink_kf[8];
static KeyFrame boot_kf[4];
static KeyFrame happy_kf[8];
static KeyFrame sad_kf[8];
static KeyFrame surprised_kf[4];
static KeyFrame thinking_kf[4];
static KeyFrame sleep_kf[6];
static KeyFrame wake_kf[6];

static Clip clip_blink     = {"blink",   0.25f, 5, false, true,  blink_kf,    2, 3};
static Clip clip_boot      = {"boot_open",0.8f, 10, false, false, boot_kf,    2, 2};
static Clip clip_happy     = {"happy",   0.5f, 3, false, true,  happy_kf,    2, 3};
static Clip clip_sad       = {"sad",     0.6f, 3, false, true,  sad_kf,      2, 3};
static Clip clip_surprised = {"surprised",0.4f, 3, false, true,  surprised_kf,2, 2};
static Clip clip_thinking  = {"thinking",0.5f, 3, false, true,  thinking_kf, 2, 2};
static Clip clip_sleep     = {"sleep_enter",1.0f, 5, false, false, sleep_kf,    2, 3};
static Clip clip_wake      = {"wake_enter",0.8f, 5, false, false, wake_kf,    2, 3};

static bool initialized = false;

static void Init() {
    if (initialized) return;
    initialized = true;
    blink_kf[0]={0.00f,0.0f,EasingType::LINEAR}; blink_kf[1]={0.12f,0.5f,EasingType::EASE_IN_QUAD}; blink_kf[2]={0.25f,0.0f,EasingType::EASE_OUT_QUAD};
    blink_kf[3]={0.00f,0.0f,EasingType::LINEAR}; blink_kf[4]={0.12f,0.5f,EasingType::EASE_IN_QUAD}; blink_kf[5]={0.25f,0.0f,EasingType::EASE_OUT_QUAD};
    boot_kf[0]={0.0f,0.5f,EasingType::EASE_OUT_QUAD}; boot_kf[1]={0.8f,0.05f,EasingType::EASE_OUT_BOUNCE};
    boot_kf[2]={0.0f,0.5f,EasingType::EASE_OUT_QUAD}; boot_kf[3]={0.8f,0.05f,EasingType::EASE_OUT_BOUNCE};
    happy_kf[0]={0.0f,0.1f,EasingType::LINEAR}; happy_kf[1]={0.25f,0.55f,EasingType::EASE_OUT_QUAD}; happy_kf[2]={0.5f,0.1f,EasingType::EASE_IN_QUAD};
    happy_kf[3]={0.0f,0.1f,EasingType::LINEAR}; happy_kf[4]={0.25f,0.55f,EasingType::EASE_OUT_QUAD}; happy_kf[5]={0.5f,0.1f,EasingType::EASE_IN_QUAD};
    sad_kf[0]={0.0f,0.0f,EasingType::LINEAR}; sad_kf[1]={0.3f,-0.3f,EasingType::EASE_OUT_QUAD}; sad_kf[2]={0.6f,0.0f,EasingType::EASE_IN_QUAD};
    sad_kf[3]={0.0f,0.0f,EasingType::LINEAR}; sad_kf[4]={0.3f,-0.3f,EasingType::EASE_OUT_QUAD}; sad_kf[5]={0.6f,0.0f,EasingType::EASE_IN_QUAD};
    surprised_kf[0]={0.0f,0.0f,EasingType::LINEAR}; surprised_kf[1]={0.4f,1.0f,EasingType::EASE_OUT_QUAD};
    surprised_kf[2]={0.0f,0.0f,EasingType::LINEAR}; surprised_kf[3]={0.4f,1.0f,EasingType::EASE_OUT_QUAD};
    thinking_kf[0]={0.0f,0.0f,EasingType::LINEAR}; thinking_kf[1]={0.5f,0.3f,EasingType::EASE_IN_OUT_QUAD};
    thinking_kf[2]={0.0f,0.0f,EasingType::LINEAR}; thinking_kf[3]={0.5f,0.3f,EasingType::EASE_IN_OUT_QUAD};
    sleep_kf[0]={0.0f,0.0f,EasingType::LINEAR}; sleep_kf[1]={0.5f,0.5f,EasingType::EASE_IN_QUAD}; sleep_kf[2]={1.0f,0.5f,EasingType::LINEAR};
    sleep_kf[3]={0.0f,0.0f,EasingType::LINEAR}; sleep_kf[4]={0.5f,0.5f,EasingType::EASE_IN_QUAD}; sleep_kf[5]={1.0f,0.5f,EasingType::LINEAR};
    wake_kf[0]={0.0f,0.5f,EasingType::LINEAR}; wake_kf[1]={0.5f,0.0f,EasingType::EASE_OUT_QUAD}; wake_kf[2]={0.8f,0.0f,EasingType::LINEAR};
    wake_kf[3]={0.0f,0.5f,EasingType::LINEAR}; wake_kf[4]={0.5f,0.0f,EasingType::EASE_OUT_QUAD}; wake_kf[5]={0.8f,0.0f,EasingType::LINEAR};
}

const Clip* AnimationLibrary::Get(const char* n)         { Init(); if(!strcmp(n,"blink"))return&clip_blink; if(!strcmp(n,"boot_open"))return&clip_boot; if(!strcmp(n,"happy"))return&clip_happy; if(!strcmp(n,"sad"))return&clip_sad; if(!strcmp(n,"surprised"))return&clip_surprised; if(!strcmp(n,"thinking"))return&clip_thinking; if(!strcmp(n,"sleep_enter"))return&clip_sleep; if(!strcmp(n,"wake_enter"))return&clip_wake; return nullptr; }
const Clip* AnimationLibrary::Happy()      { Init(); return &clip_happy; }
const Clip* AnimationLibrary::Sad()        { Init(); return &clip_sad; }
const Clip* AnimationLibrary::Angry()      { Init(); return &clip_sad; }
const Clip* AnimationLibrary::Surprised()  { Init(); return &clip_surprised; }
const Clip* AnimationLibrary::Thinking()   { Init(); return &clip_thinking; }
const Clip* AnimationLibrary::SleepEnter() { Init(); return &clip_sleep; }
const Clip* AnimationLibrary::WakeEnter()  { Init(); return &clip_wake; }
const Clip* AnimationLibrary::Blink()      { Init(); return &clip_blink; }
const Clip* AnimationLibrary::BootOpen()   { Init(); return &clip_boot; }

} // namespace procedural
