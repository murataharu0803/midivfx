#pragma once

#include "ofMain.h"

#include "PianoKey.h"
#include "ChannelState.h"
#include "VisualizerConfig.h"

class PianoKeys {
public:
	PianoKeys(std::vector<std::array<ChannelState, 16>> & channels);

	void setup();
	void draw(int64_t currentTime, const VisualizerConfig & config);

	std::vector<PianoKey> keys;

	std::vector<std::array<ChannelState, 16>> & channels;

private:
};
