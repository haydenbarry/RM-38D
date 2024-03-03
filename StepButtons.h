#pragma once
#ifndef DSY_STEP_BUTTONS_H
#define DSY_STEP_BUTTONS_H
#include "daisy.h"
#include "per/gpio.h"
#include "sys/system.h"
#include "per/spi.h"
/** 
    @author Hayden Barry
    @date Febuary 2024
*/
using namespace daisy;
class StepButtons
{
  public:

    StepButtons() {}
    ~StepButtons() {}


    void Init();
    void Debounce();

    /** \return true if a button was just pressed. */
    inline bool RisingEdge(uint8_t i) const { return updated_[i] ? state_[i] == 0x7f : false; }

    /** \return true if the button was just released */
    inline bool FallingEdge(uint8_t i) const
    {
        return updated_[i] ? state_[i] == 0x80 : false;
    }

    /** \return true if the button is held down (or if the toggle is on) */
    inline bool Pressed(uint8_t i) const { return state_[i] == 0xff; }
    bool Get(uint8_t i);
  private:
    
    uint32_t last_update_[16];
    bool     updated_[16];
    uint8_t  state_[16];
    float    rising_edge_time_[16];
    daisy::SpiHandle spi_handle;
    daisy::SpiHandle::Config spi_conf;
    daisy::GPIO latch;
    uint8_t buff[2];
    
};

#endif
