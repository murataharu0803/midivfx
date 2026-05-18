#include "PianoKeys.h"

PianoKeys::PianoKeys(
	std::vector<std::array<std::deque<noteHistory_t>, 16>> & noteHistories,
	std::vector<std::array<std::deque<channelHistory_t>, 16>> & channelHistories)
	: noteHistories(noteHistories)
	, channelHistories(channelHistories) {
	};

void PianoKeys::setup() {
	for (int i = 0; i < 128; ++i) {
		keys.emplace_back(i);
	}
}

void PianoKeys::draw(int64_t currentTime, bool reverseMode, int64_t dispatchOffset, int64_t removeOffset) {
	ofPushMatrix();
	{
		ofTranslate(-PianoKey::getKeysWidth(0, 127) / 2, 0, 0);
		for (auto & key : keys) {
			key.draw();
		}
		for (int t = 0; t < (int)noteHistories.size(); ++t) {
			for (int c = 0; c < 16; ++c) {
				auto & historyVector = noteHistories[t][c];
				auto & events = channelHistories[t][c];
				for (const auto & history : historyVector) {
					keys[history.pitch].drawHistory(currentTime, t, c, history, events, reverseMode, dispatchOffset, removeOffset);
				}
			}
		}
	}
	ofPopMatrix();
}
