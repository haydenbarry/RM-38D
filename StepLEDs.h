#pragma once
#ifndef DSY_STEP_LEDS_H
#define DSY_STEP_LEDS_H
#include "daisy.h"
#include "per/gpio.h"
#include "sys/system.h"
#include "per/spi.h"

/** 
    @author Hayden Barry
    @date Febuary 2024
*/
using namespace daisy;
class StepLEDs
{
  public:

    StepLEDs() {}
    ~StepLEDs() {}

    void Init();

    void Set(uint8_t i, bool state);
    void Set(uint16_t state);
    bool Get(uint8_t i);
    uint16_t Get();
    void Write();
    void ToggleLED(uint8_t i)  {Set(i, !Get(i));}

  private:
    
    daisy::SpiHandle spi_handle;
    daisy::SpiHandle::Config spi_conf;
    daisy::GPIO latch;
    uint8_t buff_led[2] = {0,0};
    
};

#endif
