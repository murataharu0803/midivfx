#include "PianoKeys.h"

PianoKeys::PianoKeys(std::vector<std::array<ChannelState, 16>> & channels, std::vector<BeatEvent> & beatEvents)
	: channels(channels)
	, beatEvents(beatEvents) {
	};

void PianoKeys::setup(const VisualizerConfig & config) {
	for (int i = 0; i < 128; ++i) {
		keys.emplace_back(i, config);
	}
}

void PianoKeys::draw(int64_t currentTime, const VisualizerConfig & config) {
	ofPushMatrix();
	{
		const float totalWidth = PianoKey::getKeysWidth(0, 127);
		const bool horizontal = config.horizontalMode;

		if (horizontal)
			ofTranslate(0, -totalWidth / 2, 0);
		else
			ofTranslate(-totalWidth / 2, 0, 0);

		// Draw piano keys or a simple line
		if (config.showPiano) {
			for (auto & key : keys) {
				key.draw();
			}
		} else {
			ofSetColor(ofColor(255, 255, 255, 140));
			ofSetLineWidth(2.0f);
			if (horizontal)
				ofDrawLine(0, 0, 0, 0, totalWidth, 0); // vertical line at X=0
			else
				ofDrawLine(0, 0, 0, totalWidth, 0, 0); // horizontal line at Y=0
		}

		// Draw beat lines
		if (!beatEvents.empty()) {
			const int64_t visibleStart = currentTime - config.removeOffset;
			const int64_t visibleEnd = currentTime + config.dispatchOffset;

			auto toScrollPos = [&](int64_t t) -> float {
				return config.reverseMode
					? (float)(t - currentTime) * config.speed
					: (float)(currentTime - t) * config.speed;
			};

			ofPushStyle();
			ofDisableLighting();
			for (const auto & beatEvent : beatEvents) {
				if (beatEvent.timeUs < visibleStart || beatEvent.timeUs > visibleEnd) continue;
				bool isDownbeat = (beatEvent.beatInBar == 0.0f);
				ofSetColor(isDownbeat ? ofColor(255, 255, 255, 140) : ofColor(255, 255, 255, 40));
				ofSetLineWidth(isDownbeat ? 2.0f : 1.0f);
				float pos = toScrollPos(beatEvent.timeUs);
				if (horizontal)
					ofDrawLine(pos, 0, 0, pos, totalWidth, 0);
				else
					ofDrawLine(0, pos, 0, totalWidth, pos, 0);
			}
			ofEnableLighting();
			ofPopStyle();
		}

		// Draw note histories
		for (int t = 0; t < (int)channels.size(); ++t) {
			for (int c = 0; c < 16; ++c) {
				auto & channel = channels[t][c];
				for (const auto & history : channel.noteHistories) {
					keys[history.pitch].drawHistory(currentTime, t, c, history, channel.channelHistories, config);
				}
			}
		}
	}
	ofPopMatrix();
}
