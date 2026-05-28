#pragma once

#include "ofxMidi.h"

struct keyStatus_t {
	int velocity; // also used for polyAftertouch
	bool isOn;
	bool isPedalOn;
	// TODO: CC values
};

struct noteHistory_t {
	int64_t onTime;
	int64_t offTime;
	int64_t pedalOffTime; // TODO
	uint8_t pitch;
	uint8_t velocity;
	uint32_t id = 0;
};

struct channelHistory_t {
	int64_t timestamp;
	MidiStatus status;
	uint8_t pitch; // only set for noteOn/noteOff/polyAftertouch
	uint8_t control; // only set for CC
	uint8_t value; // velocity for noteOn/noteOff/polyAftertouch, value for CC
};


struct MidiFileEvent {
	int64_t timeUs; // actual note-on/event time (relative to playbackStartTime)
	int64_t offTimeUs; // note-off time (0 if not a note-on or unlinked)
	uint8_t status;
	uint8_t track; // 0-based track index
	uint8_t channel; // 0-based MIDI channel (0-15)
	uint8_t data1;
	uint8_t data2;
};

struct BeatEvent {
	int64_t timeUs;
	float beatInBar;
};
