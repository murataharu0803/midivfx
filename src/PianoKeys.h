#pragma once

#include "ofMain.h"

#include "ChannelState.h"
#include "MidiProcessor.h"
#include "PianoKey.h"
#include "VisualizerConfig.h"

class PianoKeys {
public:
	PianoKeys(std::vector<std::array<ChannelState, 16>> & channels, std::vector<BeatEvent> & beatEvents);

	void setup(const VisualizerConfig & config);
	void setupPedalRouting(const VisualizerConfig & config, const std::vector<std::string> & trackNames, MidiProcessor & midiProcessor);
	void draw(int64_t currentTime, const VisualizerConfig & config, const std::vector<std::string> & trackNames);

	std::vector<PianoKey> keys;

	std::vector<std::array<ChannelState, 16>> & channels;
	std::vector<BeatEvent> & beatEvents;

private:
};
