#pragma once

#include "ofMain.h"

#include "PianoKey.h"

class PianoKeys {
public:
	PianoKeys(
		std::array<std::deque<noteHistory_t>, 256> & noteHistories,
		std::array<std::deque<channelHistory_t>, 256> & channelHistories);

	void setup();
	void draw(uint64_t currentTime);

	std::vector<PianoKey> keys;

	std::array<std::deque<noteHistory_t>, 256> & noteHistories;
	std::array<std::deque<channelHistory_t>, 256> & channelHistories;

private:
};
