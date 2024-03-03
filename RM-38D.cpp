#include "daisy_seed.h"
#include "daisysp.h"

#include <string.h>
#include "sys/fatfs.h"

#include "dev/oled_ssd130x.h"

#include "SampleVoice.h"
#include "StepLEDs.h"
#include "InputDriver.h"
#include "Sequencer.h"

/** 
    @author Hayden Barry
    @date Febuary 2024
*/

using namespace daisy::seed;

TimerHandle tim5_handle;
DaisySeed hw;

#define NUM_VOICES 16
#define MAX_SAMPLE_LENGTH 67108864/NUM_VOICES/sizeof(int16_t)
int16_t DSY_SDRAM_BSS sampleBuffer[NUM_VOICES][MAX_SAMPLE_LENGTH];

SampleVoice sampleVoices[NUM_VOICES];
Sequencer<16, 16, 16> sequencer;

int current_step = 0;
daisysp::Metro metro; 

class ControlChangeMsg {
public:
	uint8_t id;
	uint8_t type;
	uint8_t data[8];
	uint8_t data_size;
	enum ID {GLOBAL, GLOBAL_TEMPO, PLAY_PAUSE, STOP_START,SAMPLE_VOICE, VOICE_SEQUENCE, VOICE_PAN, VOICE_GAIN};
	enum TYPE {SET, GET, INCREMENT, DECREMENT, TOGGLE};
};

RingBuffer<ControlChangeMsg, 32> controlChangeBuffer;
RingBuffer<ControlChangeMsg, 32> controlChangedBuffer;
float GetVolume();
void InitVolumeADC();


void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size);

void initUI();

InputDriver inputDriver;
StepLEDs stepLEDs;
const int width = 128;
const int height = 64;



enum MODE {STEP, SEQUENCER, GLOBAL};
OledDisplay<SSD130xI2c128x64Driver> display;
StepLEDs leds;

void GetSampleVoice(uint8_t voice) {
	ControlChangeMsg cc;
	cc.id = ControlChangeMsg::ID::SAMPLE_VOICE;
	cc.type = ControlChangeMsg::TYPE::GET;
	cc.data[0] = voice;		
	controlChangeBuffer.Write(cc);
}

void GetSequence(uint8_t voice, uint8_t sequence) {
	ControlChangeMsg cc;
	cc.id = ControlChangeMsg::ID::VOICE_SEQUENCE;
	cc.type = ControlChangeMsg::TYPE::GET;
	cc.data[0] = voice;	
	cc.data[1] = sequence;		
	controlChangeBuffer.Write(cc);
}


void GetGlobalParameters() {
	ControlChangeMsg cc;
	cc.id = ControlChangeMsg::ID::GLOBAL;
	cc.type = ControlChangeMsg::TYPE::GET;	
	controlChangeBuffer.Write(cc);
}

void IncrementControl(uint8_t voice, uint8_t control,int8_t inc) {
	ControlChangeMsg cc;
	cc.id = control;
	if (inc < 0) cc.type = ControlChangeMsg::TYPE::DECREMENT;
	else if (inc > 0) cc.type = ControlChangeMsg::TYPE::INCREMENT;
	else return;
	cc.data[0] = voice;		

	controlChangeBuffer.Write(cc);	
}

void IncrementControl(uint8_t control,int8_t inc) {
	ControlChangeMsg cc;
	cc.id = control;
	if (inc < 0) cc.type = ControlChangeMsg::TYPE::DECREMENT;
	else if (inc > 0) cc.type = ControlChangeMsg::TYPE::INCREMENT;
	else return;

	controlChangeBuffer.Write(cc);	
}

void ToggleControl(uint8_t control) {
	ControlChangeMsg cc;
	cc.id = control;
	cc.type = ControlChangeMsg::TYPE::TOGGLE;
	
	controlChangeBuffer.Write(cc);		
}

void ToggleSequenceStep(uint8_t voice, uint8_t sequence, uint8_t step) {
	ControlChangeMsg cc;
	cc.id = ControlChangeMsg::ID::VOICE_SEQUENCE;
	cc.type = ControlChangeMsg::TYPE::TOGGLE;
	cc.data[0] = voice;
	cc.data[1] = sequence;
	cc.data[2] = step;
	controlChangeBuffer.Write(cc);		
}

struct SampleVoiceParameters {
	int8_t pan = 0;
	float gain = 0;
};
struct GlobalParameters {
	float tempo = 0;
};
struct SequenceParameters {
	uint16_t ledState = 0;
};


SdmmcHandler   sd;
FatFSInterface fsi;
DIR dir;
DIR i_dir;
FILINFO fil;
FILINFO p_fil;
FIL fp;
int num_files = 0;
char fileNames[16][FILENAME_MAX];

bool endsWith(const char *str, const char *suffix) {
  size_t str_len = strlen(str);
  size_t suffix_len = strlen(suffix);

  return (str_len >= suffix_len) && (!memcmp(str + str_len - suffix_len, suffix, suffix_len));
}

int main(void)
{
	hw.Init();

	InitVolumeADC();
	
	SdmmcHandler::Config sd_cfg;
    sd_cfg.Defaults();
    sd_cfg.width = daisy::SdmmcHandler::BusWidth::BITS_4;
    sd_cfg.speed = daisy::SdmmcHandler::Speed::FAST;
    sd.Init(sd_cfg);
	fsi.Init(FatFSInterface::Config::MEDIA_SD);
	f_mount(&fsi.GetSDFileSystem(), "/", 1);
	f_opendir(&dir, fsi.GetSDPath());

	for (unsigned int i=0; i<NUM_VOICES; i++) {
		sampleVoices[i].Init(sampleBuffer[i], MAX_SAMPLE_LENGTH);
	}

	do {
		f_readdir(&dir, &fil);

		if (fil.fname[0] == '\0') break;
		if (endsWith(fil.fname, ".wav") && !(fil.fattrib & (AM_HID|AM_DIR|AM_SYS)) && fil.fname[0] != '.') {
			sampleVoices[num_files].SetSample(fil.fname);
			strcpy(fileNames[num_files++], fil.fname);
		} 
		
	} while (num_files < 16);

	f_closedir(&dir);

	hw.SetAudioBlockSize(4); // number of samples handled per callback
	hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
	hw.StartAudio(AudioCallback);

	initUI();
	inputDriver.Init();
	stepLEDs.Init();

	sequencer.Init(hw.AudioSampleRate());



	int currentPage = STEP;
	uint8_t voice = 0;
	uint8_t sequence = 0;
	

	SampleVoiceParameters sampleVoice;
	GlobalParameters globalParameters;
	SequenceParameters sequenceParamters;
	
	bool pageChanged = true;
	GetSampleVoice(voice);
	GetSequence(voice, sequence);


	while(1) {
		inputDriver.Update();
		Input input = inputDriver.Pop();

		if (currentPage == STEP) {
			if (input.isStep() && input.down()) {
				if (input.shift()) {
					GetSampleVoice(voice = input.index);
					GetSequence(voice, sequence);
				} else {
					ToggleSequenceStep(voice, sequence, input.index);
				}

			} else if (input.IsEncoder()) {
				if (input.index == 0) {
					if (input.left()) IncrementControl(voice, ControlChangeMsg::ID::VOICE_GAIN, -1);
					else if (input.right()) IncrementControl(voice, ControlChangeMsg::ID::VOICE_GAIN, 1);
					else if (input.down() && input.shift()) {
						GetGlobalParameters();
						currentPage = GLOBAL;
					}

				} else if (input.index == 1) {
					if (input.left()) IncrementControl(voice, ControlChangeMsg::ID::VOICE_PAN, -1);
					else if (input.right()) IncrementControl(voice, ControlChangeMsg::ID::VOICE_PAN, 1);						
				}
			} else if (input.isPlay() && input.down()) {
				ToggleControl(ControlChangeMsg::ID::PLAY_PAUSE);
			} else if (input.isStop() && input.down()) {
				ToggleControl(ControlChangeMsg::ID::STOP_START);
			}	


		} else if (currentPage == GLOBAL) {
			if (input.IsEncoder()) {
				if (input.index == 0) {
					if (input.left()) IncrementControl(ControlChangeMsg::ID::GLOBAL_TEMPO, -1);
					else if (input.right()) IncrementControl(ControlChangeMsg::ID::GLOBAL_TEMPO, 1);
					else if (input.down() && input.shift()) {
						GetSampleVoice(voice);
						GetSequence(voice, sequence);
						currentPage = STEP;
					}
				}
			}
		}

		// got control change from Audio Interupt
		if (controlChangedBuffer.readable()) {
			ControlChangeMsg cc = controlChangedBuffer.ImmediateRead();
			pageChanged = true;
			if (cc.id == ControlChangeMsg::ID::SAMPLE_VOICE && cc.type == ControlChangeMsg::TYPE::GET) {
				memcpy(&sampleVoice, cc.data, sizeof(sampleVoice));
			} else if (cc.id == ControlChangeMsg::ID::GLOBAL && cc.type == ControlChangeMsg::TYPE::GET) {
				memcpy(&globalParameters, cc.data, sizeof(globalParameters));
			}  else if (cc.id == ControlChangeMsg::ID::VOICE_SEQUENCE && cc.type == ControlChangeMsg::TYPE::GET) {
				memcpy(&sequenceParamters, cc.data, sizeof(sequenceParamters));
			}
		}

		if (pageChanged) {
			pageChanged = false;
			if (currentPage == STEP) {
				char str[64];
				str[63] = '\0';
				display.Fill(false);
				sprintf(str, "V%2d      P%2d", voice, sequence);

				display.SetCursor(0,0);
				display.WriteString(str, Font_7x10, true);
				display.SetCursor(0,height/3);

				// get rid of ".wav"
				for (int i=0; i<63; i++) {
					if (fileNames[voice][i] == '\0' || fileNames[voice][i] == '.') {
						str[i] = '\0';
						break;
					} else {
						str[i] = fileNames[voice][i];
					}
				}

				display.WriteString(str, Font_7x10, true);
				
				char pan_char = ' ';
				if (sampleVoice.pan == 0) pan_char = 'C';
				else if (sampleVoice.pan < 0)  pan_char = 'L';
				else if (sampleVoice.pan > 0)  pan_char = 'R';
				int int_part = sampleVoice.gain;                  // Get the integer (678).
				float temp_frac = sampleVoice.gain - int_part;      // Get fraction (0.0123).
				int frac = trunc(temp_frac * 100);  // Turn into integer (123).
				sprintf(str, "GAIN %01d.%02d  PAN %2d%c ", int_part, frac, (sampleVoice.pan < 0) ? sampleVoice.pan*-1 : sampleVoice.pan, pan_char);
				
				display.SetCursor(0,height*2/3);
				display.WriteString(str, Font_7x10, true);
				display.Update();
				leds.Set(sequenceParamters.ledState);
				leds.Write();
			} else if (currentPage == GLOBAL) {
				display.Fill(false);
				display.SetCursor(0,(height/2)-9);
				char str[10];
				sprintf(str, "TEMPO %3d", (int)globalParameters.tempo);
				
				display.WriteString(str, Font_11x18 , true);
				display.Update();
				leds.Set(0);
				leds.Write();
			}	
		}

	}
}

void SendSampleVoice(float gain, int8_t pan) {
	SampleVoiceParameters voice =  {.pan = pan, .gain = gain};
	ControlChangeMsg cc;
	cc.id = ControlChangeMsg::ID::SAMPLE_VOICE;
	cc.type = ControlChangeMsg::TYPE::GET;
	memcpy(cc.data, &voice, sizeof(voice));
	controlChangedBuffer.Write(cc);	
}

void SendSequenceParameters(SequenceParameters sequence) {
	ControlChangeMsg cc;
	cc.id = ControlChangeMsg::ID::VOICE_SEQUENCE;
	cc.type = ControlChangeMsg::TYPE::GET;
	memcpy(cc.data, &sequence, sizeof(sequence));
	controlChangedBuffer.Write(cc);	
}

void SendGlobalParameters(GlobalParameters global) {
	ControlChangeMsg cc;
	cc.id = ControlChangeMsg::ID::GLOBAL;
	cc.type = ControlChangeMsg::TYPE::GET;
	memcpy(cc.data, &global, sizeof(global));
	controlChangedBuffer.Write(cc);	
}

bool playing = false;
uint16_t stepLED = 0;
size_t currentSequence = 0;
size_t currentVoice = 0;
void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
	// get messages from main
	if (controlChangeBuffer.readable()) {
		ControlChangeMsg cc = controlChangeBuffer.ImmediateRead();
		if (cc.id == ControlChangeMsg::ID::SAMPLE_VOICE && cc.type == ControlChangeMsg::TYPE::GET) {
			uint8_t voice = cc.data[0];
			SendSampleVoice(sampleVoices[voice].GetGain(), sampleVoices[voice].GetPan());

		} else if (cc.id == ControlChangeMsg::ID::VOICE_SEQUENCE && cc.type == ControlChangeMsg::TYPE::GET) {
			currentSequence = cc.data[1];
			currentVoice = cc.data[0];
			SendSequenceParameters({.ledState = (uint16_t)(sequencer.GetSequenceLED(currentSequence, cc.data[0]) | stepLED)});		

		} else if (cc.id == ControlChangeMsg::ID::GLOBAL && cc.type == ControlChangeMsg::TYPE::GET) {
			SendGlobalParameters({.tempo = sequencer.GetTempo()});

		} else if (cc.id == ControlChangeMsg::ID::VOICE_PAN && (cc.type == ControlChangeMsg::TYPE::INCREMENT ||  cc.type == ControlChangeMsg::TYPE::DECREMENT)) {		
			uint8_t voice = cc.data[0];
			sampleVoices[voice].IncrementPan((cc.type == ControlChangeMsg::TYPE::INCREMENT) ? 1 : -1);
			SendSampleVoice(sampleVoices[voice].GetGain(), sampleVoices[voice].GetPan());

		} else if (cc.id == ControlChangeMsg::ID::VOICE_GAIN && (cc.type == ControlChangeMsg::TYPE::INCREMENT ||  cc.type == ControlChangeMsg::TYPE::DECREMENT)) {		
			uint8_t voice = cc.data[0];
			sampleVoices[voice].IncrementGain((cc.type == ControlChangeMsg::TYPE::INCREMENT) ? 1 : -1, 0.01);
			SendSampleVoice(sampleVoices[voice].GetGain(), sampleVoices[voice].GetPan());
		} else if (cc.id == ControlChangeMsg::ID::GLOBAL_TEMPO && (cc.type == ControlChangeMsg::TYPE::INCREMENT ||  cc.type == ControlChangeMsg::TYPE::DECREMENT)) {
			sequencer.IncrementTempo(cc.type == ControlChangeMsg::TYPE::INCREMENT ? 1 : -1);
			SendGlobalParameters({.tempo = sequencer.GetTempo()});
		} else if (cc.id == ControlChangeMsg::ID::VOICE_SEQUENCE && (cc.type == ControlChangeMsg::TYPE::TOGGLE)) {
			sequencer.ToggleSequenceStep(cc.data[1], cc.data[0], cc.data[2]);
			currentSequence = cc.data[1];
			currentVoice = cc.data[0];
			SendSequenceParameters({.ledState = (uint16_t)(sequencer.GetSequenceLED(currentSequence, cc.data[0]) | stepLED)});
		} else if (cc.id == ControlChangeMsg::ID::PLAY_PAUSE && (cc.type == ControlChangeMsg::TYPE::TOGGLE)) {
			playing = !playing;
			stepLED = 0;
			SendSequenceParameters({.ledState = (uint16_t)(sequencer.GetSequenceLED(currentSequence, currentVoice))});
		} else if (cc.id == ControlChangeMsg::ID::PLAY_PAUSE && (cc.type == ControlChangeMsg::TYPE::TOGGLE)) {
			playing = !playing;
			sequencer.Restart();
			SendSequenceParameters({.ledState = (uint16_t)(sequencer.GetSequenceLED(currentSequence, currentVoice))});
		}
	}
	

	if (playing) {
		float master_vol = GetVolume();
		for (size_t i = 0; i < size; i++) {
			out[0][i] = 0;
			out[1][i] = 0;

			for (int j=0; j < 16; j++) {
				Step step = sequencer.GetNewStep(j);
				if (step.Active()) {
					sampleVoices[j].Play(step.GetVelocity());
				} 

				StereoSample samp = sampleVoices[j].StreamStereo();
				out[0][i] += samp.left;
				out[1][i] += samp.right;
			}

			out[0][i] *= master_vol;
			out[1][i] *= master_vol;

			int currentStep = sequencer.Advance();
			if (currentStep >= 0) {
				stepLED = 1U << currentStep;
				SendSequenceParameters({.ledState = (uint16_t)(sequencer.GetSequenceLED(currentSequence, currentVoice) | stepLED)});
			}
		}	

		
	} else {
		for (size_t i = 0; i < size; i++) {
			out[0][i] = 0;
			out[1][i] = 0;
		}		
	}

}



void InitVolumeADC() {
	AdcChannelConfig adc_config;
	adc_config.InitSingle(A0);
	hw.adc.Init(&adc_config, 1);
	hw.adc.Start();
}

float GetVolume() {
	return 1.0 - hw.adc.GetFloat(0);
}


void initUI() {
	leds.Init();
	leds.Set(1);

	/** Configure the Display */
	OledDisplay<SSD130xI2c128x64Driver>::Config disp_cfg;
	disp_cfg.driver_config.transport_config.i2c_address               = 0x3C;
	disp_cfg.driver_config.transport_config.i2c_config.periph         = I2CHandle::Config::Peripheral::I2C_1;
	disp_cfg.driver_config.transport_config.i2c_config.speed          = I2CHandle::Config::Speed::I2C_1MHZ;
	disp_cfg.driver_config.transport_config.i2c_config.mode           = I2CHandle::Config::Mode::I2C_MASTER;
	disp_cfg.driver_config.transport_config.i2c_config.pin_config.scl = {DSY_GPIOB, 8};    
	disp_cfg.driver_config.transport_config.i2c_config.pin_config.sda = {DSY_GPIOB, 9};

	/** And Initialize */
	display.Init(disp_cfg);
	display.Fill(true);
	display.SetCursor(0,0);
	display.WriteStringAligned("RM-38D", Font_16x26, Rectangle(width, height), Alignment::centered, false);
	display.Update();
}








