#pragma once
#ifndef DSY_INPUT_DRIVER_H
#define DSY_INPUT_DRIVER_H
#include "daisy.h"
#include "StepButtons.h"
#include "EncoderPush.h"
#include "util/ringbuffer.h"

/** 
    @author Hayden Barry
    @date Febuary 2024
*/
using namespace daisy;

struct Input {
	uint8_t id;
	uint8_t modifier;
	uint8_t action;
	uint8_t index;
	uint8_t mod_index;


	enum ID : uint8_t {NO_ID, ENC, PLAY, STOP, STEP};
	enum MOD : uint8_t {NO_MOD, SHFT, MOD_STEP};
	enum ACT : uint8_t {NO_ACT, DOWN, UP, INC, DEC};
	bool IsEncoder() {return id == ENC;}
	bool isStep() {return id == STEP;}
	bool isPlay() {return id == PLAY;}
	bool isStop() {return id == STOP;}
	bool up() {return action == UP;}
	bool down() {return action == DOWN;}
	bool right() {return action == INC;}
	bool left() {return action == DEC;}
	bool shift() {return modifier == SHFT;}
	int increment() {
		if (id == ENC && right()) return 1;
		if (id == ENC && left()) return -1;
		return 0;
	}
};

class InputDriver {
	RingBuffer<Input, 32> inputs;
	uint16_t led_state = 0;
	uint16_t new_led_state = 0;
	
	DaisySeed *hw;
	EncoderPush encoders[4];
	StepButtons step;
	Switch shift, play, stop;


	bool page_updated = false;
	bool file_browser_mode = false;
	bool browser_started = false;
	const uint8_t max_files = 64;

	uint32_t time = 0;
  void Push(Input);
public:

  void Init();


  Input Pop();
  void Update(); 


};

#endif
