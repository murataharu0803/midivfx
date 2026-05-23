#pragma once

#include <array>
#include <vector>

#include "ChannelState.h"
#include "ofxMidi.h"

class MidiProcessor {
public:
	// [track][channel] — track count set at initTracks; 16 channels fixed by MIDI spec
	std::vector<std::array<ChannelState, 16>> channels;

	void initTracks(int count);
	void processMidiMessage(ofxMidiMessage & event, uint8_t track, int64_t timestamp);

	// Returns a 128-element array: true if pitch is active on any track/channel
	std::array<bool, 128> getActiveKeys() const;

	// Removes note history entries older than (currentTime - removeOffset)
	void trimHistory(int64_t currentTime, int64_t removeOffset);

	// Returns the effective pedalOffTime for a note given a (possibly redirected)
	// pedal event source. Use when pedal.track/channel differs from the note's own.
	static int64_t resolvePedalOffTime(const noteHistory_t & history,
		const std::deque<channelHistory_t> & pedalEvents);
};
