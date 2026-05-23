#pragma once

#include <deque>

#include "VisualizerConfig.h"
#include "midiUtil.h"
#include "ofMain.h"

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
		const Style & style,
		const VisualizerConfig & config);

private:
	// Shared context precomputed once per note, used by all render modes
	struct Ctx {
		float x1, x2; // pitch bounds (vertical mode: X axis; horizontal mode: Y axis)
		int64_t tStart, tFinal, tPedalFinal;
		float velocityRatio;
		int64_t currentTime;
		const Style * style;
		const VisualizerConfig * config;

		float toScrollPos(int64_t t) const; // time → position along the scroll axis (Y or X)
	};

	static void drawCC(const Ctx & ctx, const std::deque<channelHistory_t> & events);
	static void drawCCPhase(const Ctx & ctx, const std::deque<channelHistory_t> & events,
		int64_t tFrom, int64_t tTo, const ofColor & baseColor, float z);
	static void drawDecay(const Ctx & ctx);
	static void drawDefault(const Ctx & ctx);

	// vertical mode: pitch on X, time on Y
	static void drawQuad(float x1, float x2, float yBottom, float yTop, float z,
		const ofColor & bottomColor, const ofColor & topColor);

	// horizontal mode: pitch on Y, time on X
	static void drawQuadH(float y1, float y2, float xLeft, float xRight, float z,
		const ofColor & leftColor, const ofColor & rightColor);
};
