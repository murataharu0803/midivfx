#pragma once

#include "ofMain.h"

#include "PianoKey.h"
#include "ChannelState.h"

class PianoKeys {
public:
	PianoKeys(std::vector<std::array<ChannelState, 16>> & channels);

	void setup();
	void draw(int64_t currentTime, bool reverseMode, int64_t dispatchOffset, int64_t removeOffset);

	std::vector<PianoKey> keys;

	std::vector<std::array<ChannelState, 16>> & channels;

private:
};
