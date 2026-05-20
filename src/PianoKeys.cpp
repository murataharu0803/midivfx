#include "PianoKeys.h"

PianoKeys::PianoKeys(std::vector<std::array<ChannelState, 16>> & channels)
	: channels(channels) {
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
		for (int t = 0; t < (int)channels.size(); ++t) {
			for (int c = 0; c < 16; ++c) {
				auto & channel = channels[t][c];
				for (const auto & history : channel.noteHistories) {
					keys[history.pitch].drawHistory(currentTime, t, c, history, channel.channelHistories, reverseMode, dispatchOffset, removeOffset);
				}
			}
		}
	}
	ofPopMatrix();
}
