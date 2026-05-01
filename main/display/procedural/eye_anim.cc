// procedural/eye_anim.cc
#include "types.h"
#include "scheduler.h"
#include "animation_library.h"

namespace procedural {

void TriggerBlink(Scheduler& sched) {
    sched.Play(AnimationLibrary::Blink());
}

void TriggerBootOpen(Scheduler& sched) {
    sched.Play(AnimationLibrary::BootOpen());
}

} // namespace procedural
