#pragma once

#include <deque>
#include "ofMain.h"
#include "midiUtil.h"
#include "VisualizerConfig.h"

class NoteHistoryRenderer {
public:
	static void draw(
		int64_t currentTime,
		uint8_t track,
		uint8_t channel,
		float posX,
		float width,
		const noteHistory_t & history,
		const std::deque<channelHistory_t> & events,
		const VisualizerConfig & config);

private:
	// Shared context precomputed once per note, used by all render modes
	struct Ctx {
		float x1, x2;
		int64_t tStart, tFinal, tPedalFinal;
		float velocityRatio;
		int64_t currentTime;
		const VisualizerConfig * config;

		float toY(int64_t t) const;
	};

	static void drawCC(const Ctx & ctx, const std::deque<channelHistory_t> & events);
	static void drawDecay(const Ctx & ctx);
	static void drawDefault(const Ctx & ctx);

	static void drawQuad(float x1, float x2, float yBottom, float yTop, float z,
		const ofColor & bottomColor, const ofColor & topColor);
};
