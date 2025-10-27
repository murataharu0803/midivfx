#pragma once

#include "ofxMidi.h"

struct keyStatus_t {
	int velocity; // also used for polyAftertouch
	bool isOn;
	// TODO: CC values
};

struct noteHistory_t {
	uint64_t onTime;
	uint64_t offTime;
	uint64_t pedalOffTime; // TODO
	uint8_t pitch;
	uint8_t velocity;
};

struct channelHistory_t {
	uint64_t timestamp;
	MidiStatus status;
	uint8_t pitch; // only set for noteOn/noteOff/polyAftertouch
	uint8_t control; // only set for CC
	uint8_t value; // velocity for noteOn/noteOff/polyAftertouch, value for CC
};
