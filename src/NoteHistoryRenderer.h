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

	// Mesh batch API — call beginBatch() before drawing notes, flushBatch() after
	static void beginBatch();
	static void flushBatch();

private:
	// Shared context precomputed once per note, used by all render modes
	struct Ctx {
		float pitchAxisStart, pitchAxisEnd; // pitch bounds (vertical mode: X axis; horizontal mode: Y axis)
		int64_t tStart, tFinal, tPedalFinal;
		float velocityRatio;
		float radius; // corner radius in scene units (0 = no rounding)
		int64_t currentTime;
		const Style * style;
		const VisualizerConfig * config;
		const noteHistory_t * history;

		float toScrollPos(int64_t t) const; // time → position along the scroll axis (Y or X)
	};

	struct RectAttribute {
		float x1, x2, y1, y2;
		float rx1, rx2, ry1, ry2;
	};

	static void drawCC(const Ctx & ctx, const std::deque<channelHistory_t> & events);
	static void drawCCPhase(const Ctx & ctx, const std::deque<channelHistory_t> & events,
		int64_t tFrom, int64_t tTo, const ofColor & baseColor, float z, bool isPedalPhase);
	static void drawDecay(const Ctx & ctx);
	static void drawDefault(const Ctx & ctx);

	// Rounded rect drawer
	static RectAttribute convertAxis(
		float pitchAxisStart, float pitchAxisEnd,
		int64_t timeStart, int64_t timeEnd,
		float startRadius, float endRadius,
		const Ctx & ctx);
	static void drawSegment(
		float pitchAxisStart, float pitchAxisEnd,
		int64_t timeStart, int64_t timeEnd,
		float startRadius, float endRadius,
		ofColor beginColor, ofColor endColor,
		float z, const Ctx & ctx);
	static void drawRoundedRect(
		RectAttribute attr, float z, const ofColor & color1, const ofColor & color2,
		bool gradientDirHorizontal, int segs = 8);
};
