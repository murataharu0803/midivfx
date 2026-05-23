#include <algorithm>
#include <cmath>

#include "NoteHistoryRenderer.h"

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
	float posX,
	float width,
	const noteHistory_t & history,
	const std::deque<channelHistory_t> & events,
	const Style & style,
	const VisualizerConfig & config) {

	Ctx ctx;
	ctx.x1 = posX - width / 2;
	ctx.x2 = posX + width / 2;
	ctx.tStart = std::max(history.onTime, currentTime - config.timing.removeOffset);
	ctx.tFinal = std::min(history.offTime, currentTime + config.timing.dispatchOffset);
	ctx.tPedalFinal = std::min(history.pedalOffTime, currentTime + config.timing.dispatchOffset);
	ctx.velocityRatio = style.note.velocity
		? std::pow(history.velocity / 127.f, 0.5f)
		: 1.0f;
	ctx.currentTime = currentTime;
	ctx.style = &style;
	ctx.config = &config;

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
}

// --- Render modes ---

void NoteHistoryRenderer::drawCCPhase(
	const Ctx & ctx,
	const std::deque<channelHistory_t> & events,
	int64_t tFrom, int64_t tTo,
	const ofColor & baseColor, float z) {

	const uint8_t CC_CONTROL = (uint8_t)ctx.style->note.ccNumber;

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

	int8_t ccValue = curEvent != events.end() ? curEvent->value
		: nextEvent != events.end()			  ? nextEvent->value
											  : 127;

	int64_t t = tFrom;
	while (t < tTo) {
		int64_t nextCcTime = nextEvent != events.end() ? nextEvent->timestamp : 0;
		int64_t tEnd = nextCcTime ? std::min(nextCcTime, tTo) : tTo;
		bool advance = nextEvent != events.end() && tTo >= nextCcTime;

		const float ratio = (ccValue / 127.f) * ctx.velocityRatio;
		ofColor color = baseColor;
		color.a = (uint8_t)(255 * ratio);

		if (ctx.config->display.horizontal)
			drawQuadH(ctx.x1, ctx.x2, ctx.toScrollPos(t), ctx.toScrollPos(tEnd), z, color, color);
		else
			drawQuad(ctx.x1, ctx.x2, ctx.toScrollPos(t), ctx.toScrollPos(tEnd), z, color, color);

		t = tEnd;
		if (advance) {
			ccValue = nextEvent->value;
			nextEvent = advanceToCC(++nextEvent);
		} else {
			break;
		}
	}
}

void NoteHistoryRenderer::drawCC(const Ctx & ctx, const std::deque<channelHistory_t> & events) {
	drawCCPhase(ctx, events, ctx.tStart, ctx.tFinal, ctx.style->note.color, 1.0f);

	if (ctx.style->pedal.show) {
		int64_t pedalStart = std::max(ctx.tFinal, ctx.tStart);
		drawCCPhase(ctx, events, pedalStart, ctx.tPedalFinal, ctx.style->pedal.color, 0.0f);
	}
}

void NoteHistoryRenderer::drawDecay(const Ctx & ctx) {
	const int64_t seg = ctx.style->note.decay.timeSegment;
	const float decayRate = ctx.style->note.decay.rate;
	const float z = 1.0f;
	const float pedalZ = 0.0f;

	// Note phase
	for (int64_t t = ctx.tStart; t < ctx.tFinal; t += seg) {
		int64_t tEnd = std::min(t + seg, ctx.tFinal);

		const float bottomRatio = std::pow(decayRate, (float)(t - ctx.tStart) / seg);
		const float topRatio = std::pow(decayRate, (float)(tEnd - ctx.tStart) / seg);

		ofColor bottomColor = ctx.style->note.color;
		bottomColor.a = (uint8_t)(255 * bottomRatio * ctx.velocityRatio);
		ofColor topColor = ctx.style->note.color;
		topColor.a = (uint8_t)(255 * topRatio * ctx.velocityRatio);

		if (ctx.config->display.horizontal)
			drawQuadH(ctx.x1, ctx.x2, ctx.toScrollPos(t), ctx.toScrollPos(tEnd), z, bottomColor, topColor);
		else
			drawQuad(ctx.x1, ctx.x2, ctx.toScrollPos(t), ctx.toScrollPos(tEnd), z, bottomColor, topColor);
	}

	// Pedal phase
	int64_t pedalStart = std::max(ctx.tFinal, ctx.tStart);
	for (int64_t t = pedalStart; t < ctx.tPedalFinal; t += seg) {
		int64_t tEnd = std::min(t + seg, ctx.tPedalFinal);

		const float bottomRatio = std::pow(decayRate, (float)(t - ctx.tStart) / seg);
		const float topRatio = std::pow(decayRate, (float)(tEnd - ctx.tStart) / seg);

		ofColor bottomColor = ctx.style->pedal.color;
		bottomColor.a = (uint8_t)(255 * bottomRatio * ctx.velocityRatio);
		ofColor topColor = ctx.style->pedal.color;
		topColor.a = (uint8_t)(255 * topRatio * ctx.velocityRatio);

		if (ctx.config->display.horizontal)
			drawQuadH(ctx.x1, ctx.x2, ctx.toScrollPos(t), ctx.toScrollPos(tEnd), pedalZ, bottomColor, topColor);
		else
			drawQuad(ctx.x1, ctx.x2, ctx.toScrollPos(t), ctx.toScrollPos(tEnd), pedalZ, bottomColor, topColor);
	}
}

void NoteHistoryRenderer::drawDefault(const Ctx & ctx) {
	ofColor noteColor = ctx.style->note.color;
	noteColor.a = (uint8_t)(noteColor.a * ctx.velocityRatio);
	ofColor pedalColor = ctx.style->pedal.color;

	if (ctx.config->display.horizontal) {
		const float xA = ctx.toScrollPos(ctx.tStart);
		const float xB = ctx.toScrollPos(ctx.tFinal);
		const float xC = ctx.toScrollPos(ctx.tPedalFinal);
		const float h = ctx.x2 - ctx.x1;
		const float centerY = (ctx.x1 + ctx.x2) / 2;

		ofPushStyle();
		ofSetColor(noteColor);
		ofDrawBox((xA + xB) / 2, centerY, 0, std::abs(xB - xA), h, 1);
		ofPopStyle();

		ofPushStyle();
		ofSetColor(pedalColor);
		ofDrawBox((xB + xC) / 2, centerY, 0, std::abs(xC - xB), h, 1);
		ofPopStyle();
	} else {
		const float top = ctx.toScrollPos(ctx.tStart);
		const float bottom = ctx.toScrollPos(ctx.tFinal);
		const float pedalBottom = ctx.toScrollPos(ctx.tPedalFinal);
		const float w = ctx.x2 - ctx.x1;
		const float centerX = (ctx.x1 + ctx.x2) / 2;

		ofPushStyle();
		ofSetColor(noteColor);
		ofDrawBox(centerX, (top + bottom) / 2, 0, w, top - bottom, 1);
		ofPopStyle();

		ofPushStyle();
		ofSetColor(pedalColor);
		ofDrawBox(centerX, (bottom + pedalBottom) / 2, 0, w, bottom - pedalBottom, 1);
		ofPopStyle();
	}
}

// --- Shared quad helpers ---

void NoteHistoryRenderer::drawQuad(float x1, float x2, float yBottom, float yTop, float z,
	const ofColor & bottomColor, const ofColor & topColor) {
	ofPushStyle();
	ofEnableAlphaBlending();
	{
		ofMesh mesh;
		mesh.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);

		mesh.addVertex(ofVec3f(x1, yBottom, z));
		mesh.addColor(bottomColor);
		mesh.addVertex(ofVec3f(x2, yBottom, z));
		mesh.addColor(bottomColor);

		mesh.addVertex(ofVec3f(x1, yTop, z));
		mesh.addColor(topColor);
		mesh.addVertex(ofVec3f(x2, yTop, z));
		mesh.addColor(topColor);

		mesh.draw();
	}
	ofDisableAlphaBlending();
	ofPopStyle();
}

void NoteHistoryRenderer::drawQuadH(float y1, float y2, float xLeft, float xRight, float z,
	const ofColor & leftColor, const ofColor & rightColor) {
	ofPushStyle();
	ofEnableAlphaBlending();
	{
		ofMesh mesh;
		mesh.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);

		mesh.addVertex(ofVec3f(xLeft, y1, z));
		mesh.addColor(leftColor);
		mesh.addVertex(ofVec3f(xLeft, y2, z));
		mesh.addColor(leftColor);

		mesh.addVertex(ofVec3f(xRight, y1, z));
		mesh.addColor(rightColor);
		mesh.addVertex(ofVec3f(xRight, y2, z));
		mesh.addColor(rightColor);

		mesh.draw();
	}
	ofDisableAlphaBlending();
	ofPopStyle();
}
