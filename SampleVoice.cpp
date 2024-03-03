#include "SampleVoice.h"	
	
    
char *SampleVoice::GetName() {
	return file_name;
}

// buffer should be predefined in SDRAM
void SampleVoice::Init(int16_t *buffer, uint32_t bufferLength) {
	this->buffer = buffer;
	this->bufferLength = bufferLength;
	sample_length = 0;
	startSample = 0;
	endSample = bufferLength;
	position = 0;
	
	playing = false;
}

// starts sample playing with velocity 
void SampleVoice::Play(uint8_t velocity) {
	velocity_gain = ((float)(velocity & 127)) / 127.f;
	playing = true;
	position = startSample;
}

void SampleVoice::IncrementPan(int8_t inc) {
	if (inc == -1 && pan > -50) pan--;
	else if (inc == 1 && pan < 50) pan++;
}



float SampleVoice::Stream() {
	if (playing) {
		if (position >= sample_length || position >= endSample) {
			playing = false;
			position = startSample;
			return 0.0;
		}
		return s162f(buffer[position++])*gain;
	}
	return 0.0;
}

void SampleVoice::IncrementGain(int8_t inc, float amount) {
	if (inc > 0 && gain < 1.0) gain += amount;
	else if (inc < 0 && gain > (0.0 + amount)) gain -= amount;
}

// calling this will sequence audio the same as stream
StereoSample SampleVoice::StreamStereo() {
	float samp = Stream();
	StereoSample stereoSamp;

	if (pan < 0) {
		// pan left
		stereoSamp.left = panTableB[pan*-1]*samp;
		stereoSamp.right = panTableA[pan*-1]*samp;
	} else {
		// pan right
		stereoSamp.left = panTableA[pan]*samp;
		stereoSamp.right = panTableB[pan]*samp;
	}
	return stereoSamp;
}

void SampleVoice::SetFileName(char * fname) {
	strcpy(fname, file_name);
}

void SampleVoice::IncrementStartSample(int inc) {
	if (inc < 0 && startSample > 0) startSample--;
	else if (inc > 0 && startSample < endSample && startSample < sample_length) startSample++;
}

void SampleVoice::IncrementEndSample(int inc) {
	if (inc < 0 && endSample > startSample && endSample > 0) endSample--;
	else if (inc > 0 && endSample < sample_length) startSample++;
}




void SampleVoice::SetLength(size_t sample_length) {this->sample_length = sample_length;}

void *SampleVoice::GetBuffer() {return (void *)buffer;}

uint32_t SampleVoice::GetBufferLength() {return bufferLength;}

bool SampleVoice::IsStereo() {return stereo;}
bool SampleVoice::IsMono() {return !stereo;}

/* adds given file to the buffer. Only supports 16bit PCM 48kHz Mono. 
	* If Stereo samples are interleaved left then right.
	* return 0: succesful, 1: file read failed, 2: invalid format, 3: file too large
*/ 
int SampleVoice::SetSample(TCHAR *fname) {
	UINT bytesread;
	WAV_FormatTypeDef wav_data; 
	
	memset(buffer, 0, bufferLength);
	
	if(f_open(&fp, fname, (FA_OPEN_EXISTING | FA_READ)) == FR_OK) {
		// Populate the WAV Info
		if(f_read(&fp, (void *)&wav_data, sizeof(WAV_FormatTypeDef), &bytesread) != FR_OK) return 1;	
	} else return 1;

	if (wav_data.SampleRate != 48000 || wav_data.BitPerSample != 16) return 2;
	if (wav_data.SubCHunk2Size > bufferLength || wav_data.NbrChannels > 2 || wav_data.NbrChannels != 1) return 3;
	stereo = wav_data.NbrChannels == 2;

	if (f_lseek(&fp, sizeof(WAV_FormatTypeDef)) != FR_OK) return 1;
	if(f_read(&fp, buffer, wav_data.SubCHunk2Size, &bytesread) != FR_OK) return 1;
	sample_length = bytesread / (wav_data.BitPerSample / 8);
	endSample = sample_length;
	strcpy(file_name, fname);
	f_close(&fp);
	return 0;
}