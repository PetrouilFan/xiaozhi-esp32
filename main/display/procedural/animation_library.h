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
};

} // namespace procedural