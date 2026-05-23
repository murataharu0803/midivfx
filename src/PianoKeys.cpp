#include "PianoKeys.h"
#include <algorithm>
#include <regex>

// ── Style resolution ──────────────────────────────────────────────────────────

static Style applyOverride(Style base, const StyleOverride & over) {
	if (over.note) base.note = *over.note;
	if (over.pedal) base.pedal = *over.pedal;
	base.remaps.insert(base.remaps.end(), over.remaps.begin(), over.remaps.end());
	return base;
}

static bool trackMatches(const TrackConfig & tc, int track0, const std::vector<std::string> & trackNames) {
	bool byNumber = false;
	for (int t : tc.tracks) {
		if (t - 1 == track0) {
			byNumber = true;
			break;
		}
	}
	bool byRegex = false;
	if (!tc.regex.empty() && track0 < (int)trackNames.size()) {
		try {
			std::regex re(tc.regex, std::regex_constants::icase);
			byRegex = std::regex_search(trackNames[track0], re);
		} catch (...) { }
	}
	if (tc.tracks.empty() && tc.regex.empty()) return false;
	return byNumber || byRegex;
}

static Style resolveStyle(const VisualizerConfig & config,
	const std::vector<std::string> & trackNames,
	int track0, int channel0) {

	Style result = config.style;

	for (const auto & tc : config.tracks) {
		if (!trackMatches(tc, track0, trackNames)) continue;
		result = applyOverride(result, tc.style);

		for (const auto & cc : tc.channels) {
			bool chMatch = cc.channels.empty();
			for (int c : cc.channels) {
				if (c - 1 == channel0) {
					chMatch = true;
					break;
				}
			}
			if (!chMatch) continue;
			result = applyOverride(result, cc.style);
		}
	}

	return result;
}

// ── PianoKeys ─────────────────────────────────────────────────────────────────

PianoKeys::PianoKeys(std::vector<std::array<ChannelState, 16>> & channels, std::vector<BeatEvent> & beatEvents)
	: channels(channels)
	, beatEvents(beatEvents) {
	};

void PianoKeys::setup(const VisualizerConfig & config) {
	for (int i = 0; i < 128; ++i) {
		keys.emplace_back(i, config);
	}
}

void PianoKeys::draw(int64_t currentTime, const VisualizerConfig & config, const std::vector<std::string> & trackNames) {
	ofPushMatrix();
	{
		const float totalWidth = PianoKey::getKeysWidth(0, 127);
		const bool horizontal = config.display.horizontal;

		if (horizontal)
			ofTranslate(0, -totalWidth / 2, 0);
		else
			ofTranslate(-totalWidth / 2, 0, 0);

		// Draw piano keys or a simple line
		if (config.display.showPiano) {
			for (auto & key : keys) {
				key.draw();
			}
		} else {
			ofSetColor(ofColor(255, 255, 255, 140));
			ofSetLineWidth(2.0f);
			if (horizontal)
				ofDrawLine(0, 0, 0, 0, totalWidth, 0);
			else
				ofDrawLine(0, 0, 0, totalWidth, 0, 0);
		}

		// Draw beat lines
		if (!beatEvents.empty()) {
			const int64_t visibleStart = currentTime - config.timing.removeOffset;
			const int64_t visibleEnd = currentTime + config.timing.dispatchOffset;

			auto toScrollPos = [&](int64_t t) -> float {
				return config.display.reverse
					? (float)(t - currentTime) * config.display.speed
					: (float)(currentTime - t) * config.display.speed;
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

				Style style = resolveStyle(config, trackNames, t, c);

				// Resolve pedal event source from the resolved style
				const auto & pedal = style.pedal;
				int pedalTrack = (pedal.track > 0) ? pedal.track - 1 : t;
				int pedalCh = (pedal.channel > 0) ? pedal.channel - 1 : c;
				pedalTrack = std::min(pedalTrack, (int)channels.size() - 1);
				pedalCh = std::min(pedalCh, 15);
				const auto & pedalEvents = channels[pedalTrack][pedalCh].channelHistories;

				for (const auto & history : channel.noteHistories) {
					keys[history.pitch].drawHistory(currentTime, t, c, history,
						channel.channelHistories, pedalEvents, style, config);
				}
			}
		}
	}
	ofPopMatrix();
}
