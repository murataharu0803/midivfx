#include "PianoKeys.h"

PianoKeys::PianoKeys(
	std::array<std::deque<noteHistory_t>, 256> & noteHistories,
	std::array<std::deque<channelHistory_t>, 256> & channelHistories)
	: noteHistories(noteHistories)
	, channelHistories(channelHistories) {
	};

void PianoKeys::setup() {
	for (int i = 0; i < 128; ++i) {
		keys.emplace_back(i);
	}
}

void PianoKeys::draw(uint64_t currentTime) {
	ofPushMatrix();
	{
		ofTranslate(-PianoKey::getKeysWidth(0, 127) / 2, 0, 0);
		for (auto & key : keys) {
			key.draw();
		}
		for (int i = 0; i < 256; ++i) {
			auto & historyVector = noteHistories[i];
			auto & events = channelHistories[i];
			for (const auto & history : historyVector) {
				keys[history.pitch].drawHistory(currentTime, history, events);
			}
		}
	}
	ofPopMatrix();
}
