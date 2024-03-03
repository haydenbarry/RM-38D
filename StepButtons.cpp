#include "StepButtons.h"

void StepButtons::Init()
{
    for (uint8_t i=0; i<16; i++) {
        last_update_[i] = daisy::System::GetNow();
        updated_[i]     = false;
        state_[i]       = 0x00;
    }



    spi_conf.periph = daisy::SpiHandle::Config::Peripheral::SPI_1;
	spi_conf.direction = daisy::SpiHandle::Config::Direction::TWO_LINES;
	spi_conf.pin_config.sclk = daisy::seed::D8;
	spi_conf.pin_config.miso = daisy::seed::D9;
	spi_conf.pin_config.mosi = daisy::seed::D10;
	spi_conf.pin_config.nss = Pin();
	spi_conf.nss = daisy::SpiHandle::Config::NSS::SOFT;
	spi_conf.datasize = 8;
	spi_conf.baud_prescaler = daisy::SpiHandle::Config::BaudPrescaler::PS_2;
	spi_conf.clock_polarity = daisy::SpiHandle::Config::ClockPolarity::LOW;
	spi_conf.clock_phase = daisy::SpiHandle::Config::ClockPhase::ONE_EDGE;
	

	spi_handle.Init(spi_conf);
    latch.Init(daisy::seed::D14, daisy::GPIO::Mode::OUTPUT);
    latch.Write(true);
}

bool StepButtons::Get(uint8_t i) {
    return i<8 ? (buff[0]>>i)&1 : (buff[1]>>(i%8))&1;
}

void StepButtons::Debounce()
{

    latch.Write(false);
    daisy::System::DelayTicks(1);
    latch.Write(true);
    spi_handle.BlockingReceive(buff, 2, 100);
    for (int i=0; i<16; i++) {
        
        uint32_t now = daisy::System::GetNow();
        updated_[i]     = false;

        if(now - last_update_[i] >= 1)
        {
            last_update_[i] = now;
            updated_[i]     = true;

            // shift over, and introduce new state.
            state_[i] = (state_[i] << 1) | (uint8_t)(Get(i)); // may need to invert
            // Set time at which button was pressed
            if(state_[i] == 0x7f) rising_edge_time_[i] = daisy::System::GetNow();
        }
    }

}

