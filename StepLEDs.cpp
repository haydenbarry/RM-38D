#include "StepLEDs.h"

void StepLEDs::Init()
{


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
	
	// Initialize the handle using our configuration
	spi_handle.Init(spi_conf);
    latch.Init(daisy::seed::D7, daisy::GPIO::Mode::OUTPUT);
    latch.Write(true);
    Write();
}



void StepLEDs::Write() {
    latch.Write(false);
    spi_handle.BlockingTransmit(buff_led, 2);
    latch.Write(true);
}

void StepLEDs::Set(uint16_t state) {
    buff_led[0] = state >> 8;
    buff_led[1] = state & 0xFF;
}

uint16_t StepLEDs::Get() {
    return buff_led[0]<<8 | buff_led[1];
}

void StepLEDs::Set(uint8_t i, bool state) {
    if (i >= 8) i -= 8;
    else i += 8;
    uint8_t dev, bit;
    dev = i / 8;
    bit = i % 8;
    if(state)
        buff_led[dev] |= (1 << bit);
    else
        buff_led[dev] &= ~(1 << bit);
}

bool StepLEDs::Get(uint8_t i) {
    if (i >= 8) i -= 8;
    else i += 8;
    uint8_t dev, bit;
    dev = i / 8;
    bit = i % 8;
    return buff_led[dev] & (1 << bit);
}






