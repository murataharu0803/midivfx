#pragma once

#include "ofMain.h"

#include "PianoKey.h"

class PianoKeys {
public:
	PianoKeys(
		std::vector<std::array<std::deque<noteHistory_t>, 16>> & noteHistories,
		std::vector<std::array<std::deque<channelHistory_t>, 16>> & channelHistories);

	void setup();
	void draw(uint64_t currentTime);

	std::vector<PianoKey> keys;

	std::vector<std::array<std::deque<noteHistory_t>, 16>> & noteHistories;
	std::vector<std::array<std::deque<channelHistory_t>, 16>> & channelHistories;

private:
};
