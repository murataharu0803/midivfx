#include <algorithm>
#include <cmath>

#include "NoteHistoryRenderer.h"

static constexpr float kHalfPi = 1.5707963268f;
static constexpr float kPi = 3.1415926536f;

// --- Ctx ---

float NoteHistoryRenderer::Ctx::toScrollPos(int64_t t) const {
	return config->display.reverse
		? (float)(t - currentTime) * config->display.speed
		: (float)(currentTime - t) * config->display.speed;
}

// --- Public entry point ---

void NoteHistoryRenderer::draw(
	int64_t currentTime,
	uint8_t track,
	uint8_t channel,
	float pitchAxisCenter,
	float pitchWidth,
	const noteHistory_t & history,
	const std::deque<channelHistory_t> & events,
	const Style & style,
	const VisualizerConfig & config) {

	const float gap = config.display.averageWidth
		? 328.f / 12.f * style.note.gap
		: 26.f * style.note.gap;
	const float radius = config.display.averageWidth
		? 328.f / 12.f * style.note.radius
		: 26.f * style.note.radius;

	Ctx ctx;
	ctx.pitchAxisStart = pitchAxisCenter - pitchWidth / 2.f + gap / 2.f;
	ctx.pitchAxisEnd = pitchAxisCenter + pitchWidth / 2.f - gap / 2.f;
	ctx.tStart = std::max(history.onTime, currentTime - config.timing.removeOffset);
	ctx.tFinal = std::min(history.offTime, currentTime + config.timing.dispatchOffset);
	ctx.tPedalFinal = std::min(history.pedalOffTime, currentTime + config.timing.dispatchOffset);
	ctx.velocityRatio = style.note.velocity ? std::pow(history.velocity / 127.f, 0.5f) : 1.0f;
	ctx.radius = radius;
	ctx.currentTime = currentTime;
	ctx.style = &style;
	ctx.config = &config;
	ctx.history = &history;

	switch (style.note.mode) {
	case NoteRenderMode::CC:
		drawCC(ctx, events);
		break;
	case NoteRenderMode::Decay:
		drawDecay(ctx);
		break;
	default:
		drawDefault(ctx);
		break;
	}

	if (config.debug) {
		int64_t tMid = ctx.tStart + (ctx.tFinal - ctx.tStart) / 2;
		float scrollMid = ctx.toScrollPos(tMid);
		float pitchMid = (ctx.pitchAxisStart + ctx.pitchAxisEnd) / 2.f;
		float x = config.display.horizontal ? scrollMid : pitchMid;
		float y = config.display.horizontal ? pitchMid : scrollMid;
		ofDisableDepthTest();
		ofSetColor(255, 255, 255);
		ofDrawBitmapString(std::to_string(history.id), x, y, 2.f);
		ofEnableDepthTest();
	}
}

// --- Render modes ---

void NoteHistoryRenderer::drawCCPhase(
	const Ctx & ctx,
	const std::deque<channelHistory_t> & events,
	int64_t tFrom, int64_t tTo,
	const ofColor & baseColor, float z,
	bool isPedalPhase) {

	const uint8_t CC_CONTROL = (uint8_t)ctx.style->note.ccNumber;
	const int64_t TAIL_US = 26.f / ctx.config->display.speed;

	auto advanceToCC = [&](std::deque<channelHistory_t>::const_iterator it) {
		while (it != events.end() && !(it->status == MIDI_CONTROL_CHANGE && it->control == CC_CONTROL))
			++it;
		return it;
	};

	auto nextEvent = events.begin();
	auto curEvent = events.end();
	while (nextEvent != events.end() && nextEvent->timestamp < tFrom) {
		if (nextEvent->status == MIDI_CONTROL_CHANGE && nextEvent->control == CC_CONTROL)
			curEvent = nextEvent;
		++nextEvent;
	}
	nextEvent = advanceToCC(nextEvent);

	int8_t ccValue = curEvent != events.end()
		? curEvent->value
		: nextEvent != events.end()
		? nextEvent->value
		: 127;

	int64_t t = tFrom;
	while (t < tTo) {
		int64_t nextCcTime = nextEvent != events.end() ? nextEvent->timestamp : 0;
		int64_t tEnd = nextCcTime ? std::min(nextCcTime, tTo) : tTo;
		if (tTo - tEnd < TAIL_US)
			tEnd = tTo;
		bool advance = nextEvent != events.end() && tTo >= nextCcTime;

		const float ratio = (ccValue / 127.f) * ctx.velocityRatio;
		ofColor color = baseColor;
		color.a = (uint8_t)(255 * ratio);

		float radiusStart = ctx.radius;
		float radiusEnd = ctx.radius;
		if (t != tFrom)
			radiusStart = 0.f;
		if (tEnd != tTo)
			radiusEnd = 0.f;

		if (isPedalPhase) {
			float timeAxisStart = ctx.toScrollPos(ctx.tStart);
			float timeAxisFinal = ctx.toScrollPos(ctx.tFinal);
			float timeAxisPedalEnd = ctx.toScrollPos(ctx.tPedalFinal);
			float noteLength = std::abs(timeAxisFinal - timeAxisStart);
			float pedalLength = std::abs(timeAxisPedalEnd - timeAxisFinal);
			if (t == tFrom)
				radiusStart = -noteLength / 2.f;
			if (tEnd == ctx.tPedalFinal)
				radiusEnd = std::min(noteLength / 2.f + pedalLength, radiusEnd);
		}

		drawSegment(
			ctx.pitchAxisStart, ctx.pitchAxisEnd, t, tEnd,
			radiusStart, radiusEnd, color, color, 0, ctx);

		t = tEnd;
		if (advance) {
			ccValue = nextEvent->value;
			nextEvent = advanceToCC(++nextEvent);
		} else {
			break;
		}

		if (tEnd == tTo) break;
	}
}

void NoteHistoryRenderer::drawCC(const Ctx & ctx, const std::deque<channelHistory_t> & events) {
	const bool hasPedal = ctx.style->pedal.show && (ctx.tPedalFinal > ctx.tFinal);
	drawCCPhase(ctx, events, ctx.tStart, ctx.tFinal, ctx.style->note.color, 1.0f, false);
	if (hasPedal)
		drawCCPhase(ctx, events, ctx.tFinal, ctx.tPedalFinal, ctx.style->pedal.color, 0.0f, true);
}

void NoteHistoryRenderer::drawDecay(const Ctx & ctx) {
	const int64_t seg = ctx.style->note.decay.timeSegment;
	const float decayRate = ctx.style->note.decay.rate;
	const float z = 1.0f;
	const float pedalZ = 0.0f;
	const bool hasPedal = ctx.style->pedal.show && (ctx.tPedalFinal > ctx.tFinal);
	const int64_t TAIL_US = 26.f / ctx.config->display.speed;

	// Note phase
	for (int64_t t = ctx.tStart; t < ctx.tFinal; t += seg) {
		int64_t tEnd = std::min(t + seg, ctx.tFinal);
		if (ctx.tFinal - tEnd < TAIL_US)
			tEnd = ctx.tFinal;

		const float bottomRatio = std::pow(decayRate, (float)(t - ctx.tStart) / seg);
		const float topRatio = std::pow(decayRate, (float)(tEnd - ctx.tStart) / seg);

		ofColor bottomColor = ctx.style->note.color;
		bottomColor.a = (uint8_t)(255 * bottomRatio * ctx.velocityRatio);
		ofColor topColor = ctx.style->note.color;
		topColor.a = (uint8_t)(255 * topRatio * ctx.velocityRatio);

		float radiusStart = ctx.radius;
		float radiusEnd = ctx.radius;
		if (t != ctx.tStart)
			radiusStart = 0.f;
		if (tEnd != ctx.tFinal)
			radiusEnd = 0.f;

		drawSegment(
			ctx.pitchAxisStart, ctx.pitchAxisEnd, t, tEnd,
			radiusStart, radiusEnd, bottomColor, topColor, 0, ctx);

		if (tEnd == ctx.tFinal) break;
	}

	if (!hasPedal)
		return;

	// Pedal phase
	int64_t pedalStart = std::max(ctx.tFinal, ctx.tStart);
	for (int64_t t = pedalStart; t < ctx.tPedalFinal; t += seg) {
		int64_t tEnd = std::min(t + seg, ctx.tPedalFinal);
		if (ctx.tPedalFinal - tEnd < TAIL_US)
			tEnd = ctx.tPedalFinal;

		const float bottomRatio = std::pow(decayRate, (float)(t - ctx.tStart) / seg);
		const float topRatio = std::pow(decayRate, (float)(tEnd - ctx.tStart) / seg);

		ofColor bottomColor = ctx.style->pedal.color;
		bottomColor.a = (uint8_t)(255 * bottomRatio * ctx.velocityRatio);
		ofColor topColor = ctx.style->pedal.color;
		topColor.a = (uint8_t)(255 * topRatio * ctx.velocityRatio);

		float radiusStart = ctx.radius;
		float radiusEnd = ctx.radius;
		if (t != pedalStart)
			radiusStart = 0.f;
		if (tEnd != ctx.tPedalFinal)
			radiusEnd = 0.f;

		float timeAxisStart = ctx.toScrollPos(ctx.tStart);
		float timeAxisFinal = ctx.toScrollPos(ctx.tFinal);
		float timeAxisPedalEnd = ctx.toScrollPos(ctx.tPedalFinal);
		float noteLength = std::abs(timeAxisFinal - timeAxisStart);
		float pedalLength = std::abs(timeAxisPedalEnd - timeAxisFinal);

		if (t == pedalStart)
			radiusStart = -std::min(noteLength / 2.f, radiusStart);
		if (tEnd == ctx.tPedalFinal)
			radiusEnd = std::min(noteLength / 2.f + pedalLength, radiusEnd);

		drawSegment(
			ctx.pitchAxisStart, ctx.pitchAxisEnd, t, tEnd,
			radiusStart, radiusEnd, bottomColor, topColor, 0, ctx);

		if (tEnd == ctx.tPedalFinal) break;
	}
}

void NoteHistoryRenderer::drawDefault(const Ctx & ctx) {
	ofColor noteColor = ctx.style->note.color;
	noteColor.a = (uint8_t)(noteColor.a * ctx.velocityRatio);
	ofColor pedalColor = ctx.style->pedal.color;
	const bool hasPedal = ctx.style->pedal.show && (ctx.tPedalFinal > ctx.tFinal);

	drawSegment(ctx.pitchAxisStart, ctx.pitchAxisEnd, ctx.tStart, ctx.tFinal,
		ctx.radius, ctx.radius, noteColor, noteColor, 0, ctx);
	if (hasPedal) {
		drawSegment(ctx.pitchAxisStart, ctx.pitchAxisEnd, ctx.tFinal, ctx.tPedalFinal,
			-ctx.radius, ctx.radius, pedalColor, pedalColor, 0, ctx);
	}
}

// --- Rounded rect drawer ---

NoteHistoryRenderer::RectAttribute NoteHistoryRenderer::convertAxis(
	float pitchAxisStart, float pitchAxisEnd,
	int64_t timeStart, int64_t timeEnd,
	float startRadius, float endRadius,
	const Ctx & ctx) {

	bool isHorizontal = ctx.config->display.horizontal;
	bool isReverse = ctx.config->display.reverse;

	float timeAxisStart = ctx.toScrollPos(timeStart);
	float timeAxisEnd = ctx.toScrollPos(timeEnd);
	float x1 = isHorizontal ? (isReverse ? timeAxisStart : timeAxisEnd) : ctx.pitchAxisStart;
	float x2 = isHorizontal ? (isReverse ? timeAxisEnd : timeAxisStart) : ctx.pitchAxisEnd;
	float y1 = isHorizontal ? ctx.pitchAxisStart : (isReverse ? timeAxisStart : timeAxisEnd);
	float y2 = isHorizontal ? ctx.pitchAxisEnd : (isReverse ? timeAxisEnd : timeAxisStart);
	float ry1 = isHorizontal ? ctx.radius : (isReverse ? startRadius : endRadius);
	float ry2 = isHorizontal ? ctx.radius : (isReverse ? endRadius : startRadius);
	float rx1 = !isHorizontal ? ctx.radius : (isReverse ? startRadius : endRadius);
	float rx2 = !isHorizontal ? ctx.radius : (isReverse ? endRadius : startRadius);

	RectAttribute attr = { x1, x2, y1, y2, rx1, rx2, ry1, ry2 };

	return attr;
}

void NoteHistoryRenderer::drawSegment(
	float pitchAxisStart, float pitchAxisEnd,
	int64_t timeStart, int64_t timeEnd,
	float startRadius, float endRadius,
	ofColor beginColor, ofColor endColor,
	float z, const Ctx & ctx) {

	RectAttribute attr = convertAxis(pitchAxisStart, pitchAxisEnd, timeStart, timeEnd, startRadius, endRadius, ctx);

	drawRoundedRect(attr, z, beginColor, endColor, ctx.config->display.horizontal);
}

void NoteHistoryRenderer::drawRoundedRect(
	RectAttribute attr, float z, const ofColor & color1, const ofColor & color2,
	bool gradientDirHorizontal, int segs) {

	ofPushStyle();
	ofEnableAlphaBlending();

	ofMesh mesh;
	mesh.setMode(OF_PRIMITIVE_TRIANGLES);

	const int segsZ = std::max(1, segs);

	bool xFlipped = (attr.x2 < attr.x1);
	bool yFlipped = (attr.y2 < attr.y1);
	if (xFlipped) {
		std::swap(attr.x1, attr.x2);
		std::swap(attr.rx1, attr.rx2);
	}
	if (yFlipped) {
		std::swap(attr.y1, attr.y2);
		std::swap(attr.ry1, attr.ry2);
	}

	const float cx1 = attr.x1 + attr.rx1, cx2 = attr.x2 - attr.rx2;
	const float cy1 = attr.y1 + attr.ry1, cy2 = attr.y2 - attr.ry2;

	auto colorAt = [&](float x, float y) -> ofColor {
		float t = gradientDirHorizontal
			? ((attr.x2 > attr.x1) ? std::clamp((x - attr.x1) / (attr.x2 - attr.x1), 0.f, 1.f) : 0.f)
			: ((attr.y2 > attr.y1) ? std::clamp((y - attr.y1) / (attr.y2 - attr.y1), 0.f, 1.f) : 0.f);
		return ofColor(
			(uint8_t)(color1.r + t * (color2.r - color1.r)),
			(uint8_t)(color1.g + t * (color2.g - color1.g)),
			(uint8_t)(color1.b + t * (color2.b - color1.b)),
			(uint8_t)(color1.a + t * (color2.a - color1.a)));
	};

	auto addV = [&](float x, float y) {
		mesh.addVertex(ofVec3f(x, y, z));
		mesh.addColor(colorAt(x, y));
	};
	auto addCV = [&]() {
		mesh.addVertex(ofVec3f((attr.x1 + attr.x2) / 2, (attr.y1 + attr.y2) / 2, z));
		mesh.addColor(colorAt((attr.x1 + attr.x2) / 2, (attr.y1 + attr.y2) / 2));
	};

	for (int i = 0; i < segs; i++) {
		float a1 = kPi + (segs - i) * kHalfPi / segs;
		float b1 = kPi + (segs - i - 1) * kHalfPi / segs;
		float a2 = 3 * kHalfPi + i * kHalfPi / segs;
		float b2 = 3 * kHalfPi + (i + 1) * kHalfPi / segs;

		addV(cx1 + attr.rx1 * std::cos(a1), cy1 + attr.ry1 * std::sin(a1));
		addV(cx2 + attr.rx2 * std::cos(a2), cy1 + attr.ry1 * std::sin(a2));
		addV(cx1 + attr.rx1 * std::cos(b1), cy1 + attr.ry1 * std::sin(b1));
		addV(cx2 + attr.rx2 * std::cos(a2), cy1 + attr.ry1 * std::sin(a2));
		addV(cx1 + attr.rx1 * std::cos(b1), cy1 + attr.ry1 * std::sin(b1));
		addV(cx2 + attr.rx2 * std::cos(b2), cy1 + attr.ry1 * std::sin(b2));
	}

	if (cy2 - cy1 > 0) {
		addV(attr.x1, cy1);
		addV(attr.x2, cy1);
		addV(attr.x1, cy2);
		addV(attr.x2, cy1);
		addV(attr.x1, cy2);
		addV(attr.x2, cy2);
	}

	for (int i = 0; i < segs; i++) {
		float a1 = kHalfPi + (segs - i) * kHalfPi / segs;
		float b1 = kHalfPi + (segs - i - 1) * kHalfPi / segs;
		float a2 = i * kHalfPi / segs;
		float b2 = (i + 1) * kHalfPi / segs;

		addV(cx1 + attr.rx1 * std::cos(a1), cy2 + attr.ry2 * std::sin(a1));
		addV(cx2 + attr.rx2 * std::cos(a2), cy2 + attr.ry2 * std::sin(a2));
		addV(cx1 + attr.rx1 * std::cos(b1), cy2 + attr.ry2 * std::sin(b1));
		addV(cx2 + attr.rx2 * std::cos(a2), cy2 + attr.ry2 * std::sin(a2));
		addV(cx1 + attr.rx1 * std::cos(b1), cy2 + attr.ry2 * std::sin(b1));
		addV(cx2 + attr.rx2 * std::cos(b2), cy2 + attr.ry2 * std::sin(b2));
	}

	mesh.draw();

	ofDisableAlphaBlending();
	ofPopStyle();
}
