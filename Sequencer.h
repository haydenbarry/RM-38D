#pragma once
#ifndef DSY_SEQUENCE_H
#define DSY_SEQUENCE_H
#include "daisy.h"
#include "daisysp.h"

/** 
    @author Hayden Barry
    @date Febuary 2024
*/

struct Step {

	uint8_t velocity = 100;

	// velocity ranges from 0-127
	void SetVelocity(uint8_t velocity) {
		this->velocity = (this->velocity & 128) | (velocity & 127); // velocity only 7 bit value
	}

	void IncrementVelocity() {if (GetVelocity() < 127) SetVelocity(GetVelocity() + 1);}
	void DecrementVelocity() {if (GetVelocity() > 0  ) SetVelocity(GetVelocity() - 1);}

	// velocity will be saved if set not Active
	void SetActive(bool active) {
		if (active) velocity |= 128;
		else velocity &= 127;
	}
	bool Active() {return velocity&128;}
	uint8_t GetVelocity() {return velocity & 127;}
	float GetVelocityFloat() {return (float)GetVelocity() / 127.f;}
	void ToggleActive() {SetActive(!Active());}
};

template<size_t voices, size_t steps>
class Sequence {
private:
	Step stepsArray[voices][steps];
	size_t currentStep = 0;
	bool active = false;
	bool sequenceEnd;
public: 

	void Advance() {	
		if (++currentStep >= steps) {
			sequenceEnd = true;
			currentStep = 0;
		}
	}

	bool SequenceEnded() {
		bool temp = sequenceEnd;
		sequenceEnd = false;
		return temp;
	}

	Step GetStep(size_t voiceID) {
		return stepsArray[voiceID][currentStep];
	}

	int GetCurrentStepIndex() {return currentStep;}

	void Reset() {
		currentStep = 0;
	}

	bool IsActive() {
		return active;
	}

	void SetActive(bool active) {this->active = active;}

	uint16_t GetLED(size_t voiceID) {
		uint16_t led = 0;
		for (size_t i = 0; i < steps; i++) {
			if (stepsArray[voiceID][i].Active()) led |= 1 << i;
		}
		return led;
	}

	void ToggleStep(size_t voiceID, size_t stepID) {
		stepsArray[voiceID][stepID].ToggleActive();
	}

	void SetStep(size_t voiceID, size_t stepID, bool active) {
		stepsArray[voiceID][stepID].SetActive(active);
	}

	void SetStepVelocity(size_t voiceID, size_t stepID, uint8_t velocity) {
		stepsArray[voiceID][stepID].SetVelocity(velocity);
	}

	uint8_t GetStepVelocity(size_t voiceID, size_t stepID) {
		return stepsArray[voiceID][stepID].GetVelocity();
	}

	uint8_t GetVelocity(size_t voiceID) {
		return stepsArray[voiceID][currentStep].GetVelocity();
	}

	void IncrementStepVelocity(size_t voiceID, size_t stepID, int8_t increment) {
		if (increment > 0) stepsArray[voiceID][stepID].IncrementVelocity();
		if (increment < 0) stepsArray[voiceID][stepID].DecrementVelocity();
	}

	float GetCurrentStepVelocity(uint8_t voiceID) {
		return stepsArray[voiceID][currentStep].GetVelocityFloat();
	}


};

template <size_t sequences, size_t voices, size_t steps>
class Sequencer {
	Sequence<voices, steps> sequencesArray[sequences];
	uint8_t currentSequence = 0;
    daisysp::Metro metro;
    float tempo = 0.f;
	bool playing = false;

	bool newStep[steps] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,};

    float GetTickFrequency(float tempo) {
        return 1.0 / (((60.0 / tempo) * 4.0) / steps);
    }

public:

    // call at sample rate will advance sequence if time according to tempo
	int Advance() {
        if (metro.Process()) {

            sequencesArray[currentSequence].Advance();
            if (sequencesArray[currentSequence].SequenceEnded()) {
                for (size_t i = 0; i < sequences; i++) {
                    if (sequencesArray[(i + currentSequence + 1) % sequences].IsActive()) {
                        currentSequence = (i + currentSequence +  1) % sequences;
                    }
                }
            }     
			for (size_t i = 0; i<steps; i++) {
				newStep[i] = true;
			}    
			return sequencesArray[currentSequence].GetCurrentStepIndex();
        }
		return -1;
	}

	void IncrementTempo(int8_t inc) {
		if (tempo < 999.0 && inc > 0) tempo++;
		else if (tempo > 1.0 && inc < 0) tempo--;
		else return;
		SetTempo(tempo);
	}

    void SetTempo(float tempo) {   
		this->tempo = tempo;   
        metro.SetFreq(GetTickFrequency(tempo));
    } 

	float GetTempo() {   
		return tempo;
    } 

	void Init(float sampleRate, float tempo = 120.f) {
		sequencesArray[0].SetActive(true);
        metro.Init(GetTickFrequency(tempo), sampleRate);
        this->tempo = tempo;
	}

	float GetCurrentStepVelocity(uint8_t voiceID) {
		return sequencesArray[currentSequence].GetCurrentStepVelocity(voiceID);
	}

	Step GetNewStep(uint8_t voiceID) {
        if (voiceID > sequences || !newStep[voiceID]) return {.velocity = 0};
		newStep[voiceID] = false;
		return sequencesArray[currentSequence].GetStep(voiceID);
	}

	void Restart() {
		for (size_t i = 0; i<sequences; i++) {
			sequencesArray[currentSequence].Reset();
			// set start back to first active Sequence
			if (sequencesArray[i % sequences].IsActive()) {
				currentSequence = i % sequences;
			}
		}	
		metro.Reset();
	}

	// activate or deactivate Sequence, if only one Sequence active can't deactivate 
	bool SetSequence(size_t i, bool active) {
		if (i >= sequences) return false;
		if (sequencesArray[i].IsActive() == active) return active;

		if (active) {
			sequencesArray[i].SetActive(active);
			return true;
		} else {
			int num = 0;
			for (int j=0; j<sequences; j++) {
				if (sequencesArray[j].IsActive()) num++;
			}
			if (num > 1) {
				sequencesArray[i].SetActive(false);
				return false;
			} else return false;
		}
	}

	bool GetSequenceActive(size_t sequenceID) {
		return sequencesArray[sequenceID].IsActive();
	}

	bool ToggleSequence(size_t i) {
		if (i >= sequences) return false;

		if (!sequencesArray[i].IsActive()) {
			sequencesArray[i].SetActive(true);
			return true;
		} else {
			int num = 0;
			for (int j=0; j<sequences; j++) {
				if (sequencesArray[j].IsActive()) num++;
			}
			if (num > 1) {
				sequencesArray[i].SetActive(false);
				return false;
			} else return false;
		}
	}

	void ToggleSequenceStep(size_t sequenceID, size_t voiceID, size_t stepID) {
		sequencesArray[sequenceID].ToggleStep(voiceID, stepID);
	}

	void SetSequence_step(size_t sequenceID, size_t voiceID, size_t stepID, bool active) {
		sequencesArray[sequenceID].SetStep(voiceID, stepID, active);
	}

	void SetSequenceStepVelocity(size_t sequenceID, size_t voiceID, size_t stepID, uint8_t velocity) {
		sequencesArray[sequenceID].SetStepVelocity(voiceID, stepID, velocity);
	}

	uint8_t GetStepVelocity(size_t sequenceID, size_t voiceID, size_t stepID) {
		return sequencesArray[sequenceID].GetStepVelocity(voiceID, stepID);
	}

	float GetVelocityFloat(size_t voiceID) {
		return (((float)sequencesArray[currentSequence].GetVelocity(voiceID) / 127.f) * 0.9f) + 0.1f;
	}

	void IncrementSequenceStepVelocity(size_t sequenceID, size_t voiceID, size_t stepID, int8_t increment) {
		sequencesArray[sequenceID].IncrementStepVelocity(voiceID, stepID, increment);
	}

	uint16_t GetSequenceLED(size_t sequenceID, size_t voiceID) {
		return sequencesArray[sequenceID].GetLED(voiceID);
	}

	uint16_t GetSequencerLED() {
		uint16_t seq = 0;
		for (size_t i=0; i<sequences; i++) {
			if (sequencesArray[i].IsActive()) seq |= 1U<<i;
		}
		return seq;
	}
};


#endif
