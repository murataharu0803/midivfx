#include <algorithm>
#include <cmath>

#include "NoteHistoryRenderer.h"

// --- Ctx ---

float NoteHistoryRenderer::Ctx::toScrollPos(int64_t t) const {
	return config->reverseMode
		? (float)(t - currentTime) * config->speed
		: (float)(currentTime - t) * config->speed;
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
	const VisualizerConfig & config) {

	Ctx ctx;
	ctx.x1 = posX - width / 2;
	ctx.x2 = posX + width / 2;
	ctx.tStart = std::max(history.onTime, currentTime - config.removeOffset);
	ctx.tFinal = std::min(history.offTime, currentTime + config.dispatchOffset);
	ctx.tPedalFinal = std::min(history.pedalOffTime, currentTime + config.dispatchOffset);
	ctx.velocityRatio = std::pow(history.velocity / 127.f, 0.5f);
	ctx.currentTime = currentTime;
	ctx.config = &config;

	switch (config.renderMode) {
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

void NoteHistoryRenderer::drawCC(const Ctx & ctx, const std::deque<channelHistory_t> & events) {
	const uint8_t CC_CONTROL = 0;
	const float z = 1.0f;

	auto advanceToCC = [&](std::deque<channelHistory_t>::const_iterator it) {
		while (it != events.end() && !(it->status == MIDI_CONTROL_CHANGE && it->control == CC_CONTROL))
			++it;
		return it;
	};

	// Find the last CC event before tStart; position nextEvent at first CC event at/after tStart
	auto nextEvent = events.begin();
	auto curEvent = events.end();
	while (nextEvent != events.end() && nextEvent->timestamp < ctx.tStart) {
		if (nextEvent->status == MIDI_CONTROL_CHANGE && nextEvent->control == CC_CONTROL)
			curEvent = nextEvent;
		++nextEvent;
	}
	nextEvent = advanceToCC(nextEvent);

	int8_t ccValue = curEvent != events.end() ? curEvent->value
		: nextEvent != events.end()			  ? nextEvent->value
											  : 127;
	int64_t ccEventTime = curEvent != events.end() ? curEvent->timestamp
		: nextEvent != events.end()				   ? nextEvent->timestamp
												   : 0;

	// Note phase
	int64_t tStart = ctx.tStart;
	while (tStart < ctx.tFinal) {
		int64_t nextCcTime = nextEvent != events.end() ? nextEvent->timestamp : 0;
		int64_t tEnd = nextCcTime ? std::min(nextCcTime, ctx.tFinal) : ctx.tFinal;
		bool advance = nextEvent != events.end() && ctx.tFinal >= nextCcTime;

		const float ratio = ccValue / 127.f;
		ofColor color = ctx.config->noteColor;
		color.a = 255 * ratio;
		if (ctx.config->horizontalMode)
			drawQuadH(ctx.x1, ctx.x2, ctx.toScrollPos(tStart), ctx.toScrollPos(tEnd), z, color, color);
		else
			drawQuad(ctx.x1, ctx.x2, ctx.toScrollPos(tStart), ctx.toScrollPos(tEnd), z, color, color);

		tStart = tEnd;
		if (advance) {
			ccValue = nextEvent->value;
			ccEventTime = nextEvent->timestamp;
			nextEvent = advanceToCC(++nextEvent);
		} else {
			break;
		}
	}

	// Pedal phase
	tStart = std::max(ctx.tFinal, ctx.tStart);
	while (tStart < ctx.tPedalFinal) {
		int64_t nextCcTime = nextEvent != events.end() ? nextEvent->timestamp : 0;
		int64_t tEnd = nextCcTime ? std::min(nextCcTime, ctx.tPedalFinal) : ctx.tPedalFinal;
		bool advance = nextEvent != events.end() && ctx.tPedalFinal >= nextCcTime;

		const float ratio = ccValue / 127.f;
		ofColor color = ctx.config->pedalColor;
		color.a = 255 * ratio;
		if (ctx.config->horizontalMode)
			drawQuadH(ctx.x1, ctx.x2, ctx.toScrollPos(tStart), ctx.toScrollPos(tEnd), z, color, color);
		else
			drawQuad(ctx.x1, ctx.x2, ctx.toScrollPos(tStart), ctx.toScrollPos(tEnd), z, color, color);

		tStart = tEnd;
		if (advance) {
			ccValue = nextEvent->value;
			ccEventTime = nextEvent->timestamp;
			nextEvent = advanceToCC(++nextEvent);
		} else {
			break;
		}
	}
}

void NoteHistoryRenderer::drawDecay(const Ctx & ctx) {
	const int64_t seg = ctx.config->decayTimeSegment;
	const float decayRate = ctx.config->decayRate;
	const float z = 1.0f;
	const float pedalZ = 0.0f;

	// Note phase
	for (int64_t t = ctx.tStart; t < ctx.tFinal; t += seg) {
		int64_t tEnd = std::min(t + seg, ctx.tFinal);

		const float bottomRatio = std::pow(decayRate, (float)(t - ctx.tStart) / seg);
		const float topRatio = std::pow(decayRate, (float)(tEnd - ctx.tStart) / seg);

		ofColor bottomColor = ctx.config->noteColor;
		bottomColor.a = 255 * bottomRatio * ctx.velocityRatio;
		ofColor topColor = ctx.config->noteColor;
		topColor.a = 255 * topRatio * ctx.velocityRatio;

		if (ctx.config->horizontalMode)
			drawQuadH(ctx.x1, ctx.x2, ctx.toScrollPos(t), ctx.toScrollPos(tEnd), z, bottomColor, topColor);
		else
			drawQuad(ctx.x1, ctx.x2, ctx.toScrollPos(t), ctx.toScrollPos(tEnd), z, bottomColor, topColor);
	}

	// Pedal phase
	int64_t pedalStart = std::max(ctx.tFinal, ctx.tStart);
	for (int64_t t = pedalStart; t < ctx.tPedalFinal; t += seg) {
		int64_t tEnd = std::min(t + seg, ctx.tPedalFinal);

		// Decay continues from note-on time, not pedal start
		const float bottomRatio = std::pow(decayRate, (float)(t - ctx.tStart) / seg);
		const float topRatio = std::pow(decayRate, (float)(tEnd - ctx.tStart) / seg);

		ofColor bottomColor = ctx.config->pedalColor;
		bottomColor.a = 255 * bottomRatio * ctx.velocityRatio;
		ofColor topColor = ctx.config->pedalColor;
		topColor.a = 255 * topRatio * ctx.velocityRatio;

		if (ctx.config->horizontalMode)
			drawQuadH(ctx.x1, ctx.x2, ctx.toScrollPos(t), ctx.toScrollPos(tEnd), pedalZ, bottomColor, topColor);
		else
			drawQuad(ctx.x1, ctx.x2, ctx.toScrollPos(t), ctx.toScrollPos(tEnd), pedalZ, bottomColor, topColor);
	}
}

void NoteHistoryRenderer::drawDefault(const Ctx & ctx) {
	if (ctx.config->horizontalMode) {
		const float xA = ctx.toScrollPos(ctx.tStart);
		const float xB = ctx.toScrollPos(ctx.tFinal);
		const float xC = ctx.toScrollPos(ctx.tPedalFinal);
		const float h = ctx.x2 - ctx.x1;
		const float centerY = (ctx.x1 + ctx.x2) / 2;

		ofPushStyle();
		ofSetColor(ctx.config->noteColor);
		ofDrawBox((xA + xB) / 2, centerY, 0, std::abs(xB - xA), h, 1);
		ofPopStyle();

		ofPushStyle();
		ofSetColor(ctx.config->pedalColor);
		ofDrawBox((xB + xC) / 2, centerY, 0, std::abs(xC - xB), h, 1);
		ofPopStyle();
	} else {
		const float top = ctx.toScrollPos(ctx.tStart);
		const float bottom = ctx.toScrollPos(ctx.tFinal);
		const float pedalBottom = ctx.toScrollPos(ctx.tPedalFinal);
		const float w = ctx.x2 - ctx.x1;
		const float centerX = (ctx.x1 + ctx.x2) / 2;

		ofPushStyle();
		ofSetColor(ctx.config->noteColor);
		ofDrawBox(centerX, (top + bottom) / 2, 0, w, top - bottom, 1);
		ofPopStyle();

		ofPushStyle();
		ofSetColor(ctx.config->pedalColor);
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

// Horizontal mode: y1/y2 are pitch bounds, xLeft/xRight are time bounds.
// leftColor = color at older time (left), rightColor = color at newer time (right/piano side).
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
