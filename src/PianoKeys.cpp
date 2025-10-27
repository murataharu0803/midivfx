#include "PianoKeys.h"

PianoKeys::PianoKeys(std::array<std::deque<noteHistory_t>, 256> & noteHistories)
	: noteHistories(noteHistories) {
}

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
		for (auto & historyVector : noteHistories) {
			for (const auto & history : historyVector) {
				keys[history.pitch].drawHistory(history, currentTime);
			}
		}
	}
	ofPopMatrix();
}
