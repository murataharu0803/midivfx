#pragma once

#include "ofMain.h"

#include "PianoKey.h"

class PianoKeys {
public:
	PianoKeys(
		std::vector<std::array<std::deque<noteHistory_t>, 16>> & noteHistories,
		std::vector<std::array<std::deque<channelHistory_t>, 16>> & channelHistories);

	void setup();
	void draw(int64_t currentTime, bool reverseMode, int64_t dispatchOffset, int64_t removeOffset);

	std::vector<PianoKey> keys;

	std::vector<std::array<std::deque<noteHistory_t>, 16>> & noteHistories;
	std::vector<std::array<std::deque<channelHistory_t>, 16>> & channelHistories;

private:
};
