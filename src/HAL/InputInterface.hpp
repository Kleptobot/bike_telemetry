#pragma once
#include <Arduino.h>

typedef struct {
    bool state;             //current button state
    bool RE;                //rising edge
    bool FE;                //falling edge
    bool press;             //a RE and FE detected within a set duration
    bool held;              //button held for longer than threshold
    uint32_t heldTime;      //button held for this time
    uint32_t releaseTime;   //button unheld for this time

} buttonState;

typedef struct {
    buttonState Up;
    buttonState Down;
    buttonState Left;
    buttonState Right;
    buttonState Select;
    buttonState SD_Det;
} physIO;