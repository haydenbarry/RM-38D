 #include "EncoderPush.h"
 
    void EncoderPush::Init(Pin pin_a, Pin pin_b, Pin pin_s) {
        a.Init(pin_a, daisy::GPIO::Mode::INPUT, daisy::GPIO::Pull::PULLUP);
        b.Init(pin_b, daisy::GPIO::Mode::INPUT, daisy::GPIO::Pull::PULLUP);
        s.Init(pin_s);
        prevNextCode = 0;
        store = 0;
    }

    

    void EncoderPush::Update() {
        s.Debounce();
    }

    bool EncoderPush::Rising() {
        return s.RisingEdge();
    }

    bool EncoderPush::Falling() {
        return s.FallingEdge();
    }

    bool EncoderPush::Pressed() {
        return s.Pressed();
    }

    // A vald CW or  CCW move returns 1, invalid returns 0.
    int8_t EncoderPush::ReadRotary() {

        prevNextCode <<= 2;
        if (a.Read()) prevNextCode |= 0x02;
        if (b.Read()) prevNextCode |= 0x01;
        prevNextCode &= 0x0f;

        // If valid then store as 16 bit data.
        if  (rot_enc_table[prevNextCode] ) {
            store <<= 4;
            store |= prevNextCode;
            //if (store==0xd42b) return 1;
            //if (store==0xe817) return -1;
            if ((store&0xff)==0x2b) return -1;
            if ((store&0xff)==0x17) return 1;
        }
        return 0;
    }