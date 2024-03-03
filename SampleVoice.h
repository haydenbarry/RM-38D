#pragma once
#ifndef DSY_SAMPLE_VOICE_H
#define DSY_SAMPLE_VOICE_H
#include "daisy.h"

/** 
    @author Hayden Barry
    @date Febuary 2024
*/

using namespace daisy;

typedef struct {
	float left;
	float right;
} StereoSample;

class SampleVoice {
private:

	uint32_t bufferLength;


	size_t sample_length;
	size_t position;
	size_t startSample = 0;
	size_t endSample = 0;

	bool playing;
	int16_t *buffer;
	FIL fp;
	char file_name[256];
	bool stereo;
	float gain = 0.5;
	float velocity_gain = 1.0;
	

	/* A=\frac{\sqrt{2}}{2}(\cos x+\sin x)\left\{0\le x\le\frac{\pi}{4}\right\}
	   B=\frac{\sqrt{2}}{2}(\cos x-\sin x)\left\{0\le x\le\frac{\pi}{4}\right\}
	*/ 
	const float panTableA[51] = {0.7071067812,0.7181262978,0.7289686274,0.739631095,0.7501110696,0.7604059656,0.7705132428,0.7804304073,0.7901550124,0.7996846585,0.8090169944,0.8181497174,0.8270805743,0.8358073614,0.8443279255,0.8526401644,0.860742027,0.8686315144,0.87630668,0.8837656301,0.8910065242,0.8980275758,0.9048270525,0.9114032766,0.9177546257,0.9238795325,0.9297764859,0.9354440308,0.940880769,0.9460853588,0.9510565163,0.9557930148,0.9602936857,0.9645574185,0.9685831611,0.9723699204,0.9759167619,0.9792228106,0.9822872507,0.9851093262,0.9876883406,0.9900236577,0.9921147013,0.9939609555,0.9955619646,0.9969173337,0.9980267284,0.998889875,0.9995065604,0.9998766325,1};
	const float panTableB[51] = {0.7071067812,0.6959127966,0.6845471059,0.6730125135,0.6613118653,0.6494480483,0.6374239897,0.6252426563,0.6129070537,0.6004202253,0.5877852523,0.575005252,0.5620833779,0.549022818,0.535826795,0.5224985647,0.5090414158,0.4954586684,0.4817536741,0.4679298143,0.4539904997,0.4399391699,0.4257792916,0.4115143586,0.3971478906,0.3826834324,0.3681245527,0.3534748438,0.3387379202,0.3239174182,0.3090169944,0.2940403252,0.278991106,0.26387305,0.2486898872,0.2334453639,0.2181432414,0.2027872954,0.1873813146,0.1719291003,0.156434465,0.1409012319,0.1253332336,0.1097343111,0.09410831332,0.07845909573,0.06279051953,0.04710645071,0.03141075908,0.01570731731,0};
	int8_t pan = 0;
public:

	char *GetName();
	// buffer should be predefined in SDRAM
	void Init(int16_t *buffer, uint32_t bufferLength);

	// starts sample playing. need to add handling of sample already playing.
	void Play(uint8_t velocity);

	int8_t prev = 0;
	void IncrementPan(int8_t inc);
	void IncrementGain(int8_t inc, float amount);	
	void IncrementStartSample(int inc);
	void IncrementEndSample(int inc);

	float GetGain() {return gain;}
	int8_t GetPan() {return pan;}


	float Stream();

	// calling this will sequence audio the same as stream
	StereoSample StreamStereo();

	void SetFileName(char * fname);

	void SetLength(size_t sample_length);

	void *GetBuffer(); 

	uint32_t GetBufferLength();

	bool IsStereo();
	bool IsMono();

	/* adds given file to the buffer. Only supports 16bit PCM 48kHz Mono. 
	 * If Stereo samples are interleaved left then right.
	 * return 0: succesful, 1: file read failed, 2: invalid format, 3: file too large
	*/ 
	int SetSample(TCHAR *fname);
};
#endif
