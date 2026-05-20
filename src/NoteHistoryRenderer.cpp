#include <algorithm>
#include <cmath>

#include "NoteHistoryRenderer.h"

static const float speed = .001f;

static const ofColor BASE_COLOR(255, 64, 0);
static const ofColor PEDAL_COLOR(128, 128, 128);

// --- Ctx ---

float NoteHistoryRenderer::Ctx::toY(int64_t t) const {
	return reverseMode
		? (float)(t - currentTime) * speed
		: (float)(currentTime - t) * speed;
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
	bool reverseMode,
	int64_t dispatchOffset,
	int64_t removeOffset,
	NoteRenderMode mode) {

	Ctx ctx;
	ctx.x1 = posX - width / 2;
	ctx.x2 = posX + width / 2;
	ctx.tStart = std::max(history.onTime, currentTime - removeOffset);
	ctx.tFinal = std::min(history.offTime, currentTime + dispatchOffset);
	ctx.tPedalFinal = std::min(history.pedalOffTime, currentTime + dispatchOffset);
	ctx.velocityRatio = std::pow(history.velocity / 127.f, 0.5f);
	ctx.currentTime = currentTime;
	ctx.reverseMode = reverseMode;

	switch (mode) {
		case NoteRenderMode::CC:    drawCC(ctx, events); break;
		case NoteRenderMode::Decay: drawDecay(ctx); break;
		default:                    drawDefault(ctx); break;
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
		: nextEvent != events.end() ? nextEvent->value : 127;
	int64_t ccEventTime = curEvent != events.end() ? curEvent->timestamp
		: nextEvent != events.end() ? nextEvent->timestamp : 0;

	// Note phase
	int64_t tStart = ctx.tStart;
	while (tStart < ctx.tFinal) {
		int64_t nextCcTime = nextEvent != events.end() ? nextEvent->timestamp : 0;
		int64_t tEnd = nextCcTime ? std::min(nextCcTime, ctx.tFinal) : ctx.tFinal;
		bool advance = nextEvent != events.end() && ctx.tFinal >= nextCcTime;

		const float ratio = ccValue / 127.f;
		ofColor color = BASE_COLOR;
		color.a = 255 * ratio;
		drawQuad(ctx.x1, ctx.x2, ctx.toY(tStart), ctx.toY(tEnd), z, color, color);

		tStart = tEnd;
		if (advance) {
			ccValue = nextEvent->value;
			ccEventTime = nextEvent->timestamp;
			nextEvent = advanceToCC(++nextEvent);
		} else {
			break;
		}
	}

	// Pedal phase (same z as note in CC mode)
	tStart = std::max(ctx.tFinal, ctx.currentTime - (ctx.currentTime - ctx.tStart)); // reset
	tStart = std::max(ctx.tFinal, ctx.tStart); // simpler: restart from note end
	while (tStart < ctx.tPedalFinal) {
		int64_t nextCcTime = nextEvent != events.end() ? nextEvent->timestamp : 0;
		int64_t tEnd = nextCcTime ? std::min(nextCcTime, ctx.tPedalFinal) : ctx.tPedalFinal;
		bool advance = nextEvent != events.end() && ctx.tPedalFinal >= nextCcTime;

		const float ratio = ccValue / 127.f;
		ofColor color = PEDAL_COLOR;
		color.a = 255 * ratio;
		drawQuad(ctx.x1, ctx.x2, ctx.toY(tStart), ctx.toY(tEnd), z, color, color);

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
	const int64_t TIME_SEGMENT = 100000; // us
	const float DECAY_RATE = 0.95f;
	const float z = 1.0f;
	const float pedalZ = 0.0f;

	// Note phase
	for (int64_t t = ctx.tStart; t < ctx.tFinal; t += TIME_SEGMENT) {
		int64_t tEnd = std::min(t + TIME_SEGMENT, ctx.tFinal);

		const float bottomRatio = std::pow(DECAY_RATE, (float)(t - ctx.tStart) / TIME_SEGMENT);
		const float topRatio = std::pow(DECAY_RATE, (float)(tEnd - ctx.tStart) / TIME_SEGMENT);

		ofColor bottomColor = BASE_COLOR;
		bottomColor.a = 255 * bottomRatio * ctx.velocityRatio;
		ofColor topColor = BASE_COLOR;
		topColor.a = 255 * topRatio * ctx.velocityRatio;

		drawQuad(ctx.x1, ctx.x2, ctx.toY(t), ctx.toY(tEnd), z, bottomColor, topColor);
	}

	// Pedal phase
	int64_t pedalStart = std::max(ctx.tFinal, ctx.tStart);
	for (int64_t t = pedalStart; t < ctx.tPedalFinal; t += TIME_SEGMENT) {
		int64_t tEnd = std::min(t + TIME_SEGMENT, ctx.tPedalFinal);

		// Decay continues from note-on time, not pedal start
		const float bottomRatio = std::pow(DECAY_RATE, (float)(t - ctx.tStart) / TIME_SEGMENT);
		const float topRatio = std::pow(DECAY_RATE, (float)(tEnd - ctx.tStart) / TIME_SEGMENT);

		ofColor bottomColor = PEDAL_COLOR;
		bottomColor.a = 255 * bottomRatio * ctx.velocityRatio;
		ofColor topColor = PEDAL_COLOR;
		topColor.a = 255 * topRatio * ctx.velocityRatio;

		drawQuad(ctx.x1, ctx.x2, ctx.toY(t), ctx.toY(tEnd), pedalZ, bottomColor, topColor);
	}
}

void NoteHistoryRenderer::drawDefault(const Ctx & ctx) {
	const float top = ctx.toY(ctx.tStart);
	const float bottom = ctx.toY(ctx.tFinal);
	const float pedalBottom = ctx.toY(ctx.tPedalFinal);
	const float w = ctx.x2 - ctx.x1;
	const float centerX = (ctx.x1 + ctx.x2) / 2;

	ofPushStyle();
	ofSetColor(BASE_COLOR);
	ofDrawBox(centerX, (top + bottom) / 2, 0, w, top - bottom, 1);
	ofPopStyle();

	ofPushStyle();
	ofSetColor(PEDAL_COLOR);
	ofDrawBox(centerX, (bottom + pedalBottom) / 2, 0, w, bottom - pedalBottom, 1);
	ofPopStyle();
}

// --- Shared quad helper ---

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
