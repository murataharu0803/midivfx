#pragma once

#include <array>
#include <map>
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

	// Sets pedal redirection routing: maps (track, channel) → (pedalTrack, pedalChannel).
	// Only include entries where the pedal source differs from the note channel.
	// processMidiMessage will use this to apply pedal events to subscriber channels.
	void setPedalRouting(const std::map<std::pair<int,int>, std::pair<int,int>> & routing);

private:
	// Forward: (track, channel) → (pedalTrack, pedalChannel)
	std::map<std::pair<int,int>, std::pair<int,int>> pedalRouting;
	// Reverse: (pedalTrack, pedalChannel) → channels that subscribe to it
	std::map<std::pair<int,int>, std::vector<std::pair<int,int>>> pedalSubscribers;
};
