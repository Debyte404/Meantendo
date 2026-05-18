#pragma once
#include <Arduino.h>

namespace Buzzer {

    enum class Sound {
        Click,
        Confirm,
        Error
    };

    void init();
    void play(Sound s);
}
