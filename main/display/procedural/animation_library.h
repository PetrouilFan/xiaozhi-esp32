#pragma once
#include "types.h"

namespace procedural {

class AnimationLibrary {
public:
    static const Clip* Get(const char* name);
    static const Clip* Happy();
    static const Clip* Sad();
    static const Clip* Angry();
    static const Clip* Surprised();
    static const Clip* Thinking();
    static const Clip* SleepEnter();
    static const Clip* WakeEnter();
    static const Clip* Blink();
    static const Clip* BootOpen();
    static const Clip* Breathing();
    static const Clip* MicroTilt();
    static const Clip* SlowBlink();
    static const Clip* DoubleBlink();
    static const Clip* SleepIdle();
    static const Clip* TestHorizontal();
    static const Clip* TinyFocusNarrow();
};

} // namespace procedural