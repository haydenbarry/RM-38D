#include "InputDriver.h"

void InputDriver::Init() {
    step.Init();
    encoders[0].Init(seed::D23, seed::D22, seed::D24);
    encoders[1].Init(seed::D26, seed::D25, seed::D27);
    encoders[2].Init(seed::D20, seed::D19, seed::D21);
    encoders[3].Init(seed::D17, seed::D16, seed::D18);
    

    shift.Init(seed::D30);
    play.Init(seed::D29);
    stop.Init(seed::D28);
    inputs.Init();
    
}


// to be called in main loop 
void InputDriver::Update() {

    play.Debounce();
    stop.Debounce();
    shift.Debounce();
    step.Debounce();
    Input input = {0,0,0,0};
    int temp;
    int step_pressed = -1;

    if (play.FallingEdge()) Push({.id = Input::ID::PLAY, .modifier = shift.Pressed() ? Input::MOD::SHFT : Input::MOD::NO_MOD, .action = Input::ACT::UP, .index = 0});
    if (play.RisingEdge()) Push({.id = Input::ID::PLAY, .modifier = shift.Pressed() ? Input::MOD::SHFT : Input::MOD::NO_MOD, .action = Input::ACT::DOWN, .index = 0});

    if (stop.FallingEdge()) Push({.id = Input::ID::STOP, .modifier = shift.Pressed() ? Input::MOD::SHFT : Input::MOD::NO_MOD, .action = Input::ACT::UP, .index = 0});
    if (stop.RisingEdge()) Push({.id = Input::ID::STOP, .modifier = shift.Pressed() ? Input::MOD::SHFT : Input::MOD::NO_MOD, .action = Input::ACT::DOWN, .index = 0});

    for (uint8_t i=0; i<16; i++) {
        if (step.Get(i)) step_pressed = i; // used as modifier for encoders
        if (step.RisingEdge(i)) Push({.id = (uint8_t)(Input::ID::STEP), input.modifier = shift.Pressed() ? Input::MOD::SHFT : Input::MOD::NO_MOD, .action = Input::ACT::DOWN, .index = i});
        if (step.FallingEdge(i)) Push({.id = (uint8_t)(Input::ID::STEP), input.modifier = shift.Pressed() ? Input::MOD::SHFT : Input::MOD::NO_MOD, .action = Input::ACT::UP, .index = i});
    }

    for (uint8_t i=0; i<4; i++) {
        if ((temp = encoders[i].ReadRotary())) {
            input.id = Input::ID::ENC;
            input.index = i;

            if (shift.Pressed()) input.modifier = Input::MOD::SHFT;
            else if (step_pressed != -1) {
                input.modifier = Input::MOD::MOD_STEP;
                input.mod_index = i;
            } else input.modifier = Input::MOD::NO_MOD;

            if (temp == 1) input.action = Input::ACT::INC;
            else input.action = Input::ACT::DEC;
            Push(input);
        }

        encoders[i].Update();
        if (encoders[i].Rising()) {
            input.id = Input::ID::ENC;
            input.index = i;
            if (shift.Pressed()) input.modifier = Input::MOD::SHFT;
            else input.modifier = Input::MOD::NO_MOD;

            input.action = Input::ACT::DOWN;
            Push(input);
        }

        if (encoders[i].Falling()) {
            input.id = Input::ID::ENC;
            input.index = i;
            if (shift.Pressed()) input.modifier = Input::MOD::SHFT;
            else input.modifier = Input::MOD::NO_MOD;

            input.action = Input::ACT::UP;
            Push(input);
        }
    }
}

void InputDriver::Push(Input input) {
    inputs.Write(input);
} 

// called in interupt to process input 
Input InputDriver::Pop() {
    if (inputs.readable()) return inputs.Read();
    else return {.id = Input::ID::NO_ID, .modifier = Input::MOD::NO_MOD, .action = Input::ACT::NO_ACT, .index = 0};
}




