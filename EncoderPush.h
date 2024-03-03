#pragma once
#ifndef EncoderPush_H
#define EncoderPush165_H

#include "daisy_seed.h"

/** 
    @author Hayden Barry
    @date Febuary 2024
*/

class EncoderPush {
private:
    uint8_t prevNextCode;
    uint16_t store;
    GPIO a, b;
    Switch s;
    const int8_t rot_enc_table[16] = {0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0};

public:
    void Init(Pin pin_a, Pin pin_b, Pin pin_s);
    void Update();
    bool Rising();
    bool Falling();
    bool Pressed();
    int8_t ReadRotary();
   
};

#endif