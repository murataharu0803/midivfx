#include "PianoKey.h"

// layout constants (exposed externally for Waterfall)
const float SCALE = 2.f;
const float OCTAVE_WIDTH = 164 * SCALE;
const float WHITE_KEY_WIDTH = 23 * SCALE;
const float BLACK_KEY_WIDTH = 14 * SCALE;
const float WHITE_KEY_HEIGHT = 150 * SCALE;
const float BLACK_KEY_HEIGHT = 100 * SCALE;

const float padding = 2.f; // padding between keys
const float speed = .001f;

// Which notes are black keys
const bool BLACK_KEY_PATTERN[12] = {
	false, true, false, true, false, // C, C#, D, D#, E
	false, true, false, true, false, true, false // F, F#, G, G#, A, A#, B
};

const float KEY_ROOT_WIDTHS[12] = {
	14 * SCALE,
	14 * SCALE,
	14 * SCALE,
	14 * SCALE,
	14 * SCALE,
	13 * SCALE,
	14 * SCALE,
	13 * SCALE,
	14 * SCALE,
	13 * SCALE,
	14 * SCALE,
	13 * SCALE,
};

const float KEY_END_WIDTHS[12] = {
	23 * SCALE,
	14 * SCALE,
	24 * SCALE,
	14 * SCALE,
	23 * SCALE,
	24 * SCALE,
	14 * SCALE,
	23 * SCALE,
	14 * SCALE,
	23 * SCALE,
	14 * SCALE,
	24 * SCALE,
};

const float KEY_END_OFFSETS[12] = {
	0 * SCALE,
	14 * SCALE,
	23 * SCALE,
	42 * SCALE,
	47 * SCALE,
	70 * SCALE,
	83 * SCALE,
	94 * SCALE,
	110 * SCALE,
	117 * SCALE,
	137 * SCALE,
	140 * SCALE,
};

// center position for notes in waterfall
const float NOTE_CENTER_OFFSETS[12] = {
	7 * SCALE,
	21 * SCALE,
	35 * SCALE,
	49 * SCALE,
	63 * SCALE,
	76.5 * SCALE,
	90 * SCALE,
	103.5 * SCALE,
	117 * SCALE,
	130.5 * SCALE,
	144 * SCALE,
	157.5 * SCALE,
};

const std::array<percussionMapping_t, 1> percussionMappings = {
	{ 0, 36, 24, 36, false }, // map C2 to all C notes in percussion channel
};

PianoKey::PianoKey(uint8_t note)
	: noteNumber(note)
	, isActive(false) {
	int noteInOctave = note % 12;
	isBlackKey = BLACK_KEY_PATTERN[noteInOctave];

	posX = calculatePosition(noteNumber);
	rootPosX = calculateRootCenter(noteNumber);
	width = calculateWidth(noteNumber);
	height = calculateHeight(noteNumber);

	// Set color
	color = isBlackKey ? ofColor(30, 30, 30) : ofColor(240, 240, 240);
}

float PianoKey::getKeysWidth(int begin, int end) {
	int beginOctave = begin / 12;
	int endOctave = end / 12;
	int beginNoteInOctave = begin % 12;
	int endNoteInOctave = end % 12;

	float width = (endOctave - beginOctave) * OCTAVE_WIDTH;
	width += KEY_END_OFFSETS[endNoteInOctave] - KEY_END_OFFSETS[beginNoteInOctave];
	return width + KEY_END_WIDTHS[endNoteInOctave];
}

float PianoKey::calculatePosition(uint8_t noteNumber) {
	int octave = noteNumber / 12;
	int noteInOctave = noteNumber % 12;
	return KEY_END_OFFSETS[noteInOctave] + octave * OCTAVE_WIDTH;
}

float PianoKey::calculateRootCenter(uint8_t noteNumber) {
	int octave = noteNumber / 12;
	int noteInOctave = noteNumber % 12;
	return NOTE_CENTER_OFFSETS[noteInOctave] + octave * OCTAVE_WIDTH;
}

float PianoKey::calculateWidth(uint8_t noteNumber) {
	int noteInOctave = noteNumber % 12;
	return KEY_END_WIDTHS[noteInOctave];
}

float PianoKey::calculateHeight(uint8_t noteNumber) {
	int noteInOctave = noteNumber % 12;
	return BLACK_KEY_PATTERN[noteInOctave] ? BLACK_KEY_HEIGHT : WHITE_KEY_HEIGHT;
}

void PianoKey::draw() {
	float w = isBlackKey ? width : width - padding;
	float h = height;
	float x = posX + width / 2;
	float y = -height / 2;
	float z = isBlackKey ? 1 : 0;

	ofPushStyle();
	{
		ofMesh mesh;
		mesh.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);

		mesh.addVertex(ofVec3f(x - w / 2, y - h / 2, z));
		mesh.addNormal(ofVec3f(0, 0, 1));
		mesh.addColor(color);

		mesh.addVertex(ofVec3f(x + w / 2, y - h / 2, z));
		mesh.addNormal(ofVec3f(0, 0, 1));
		mesh.addColor(color);

		mesh.addVertex(ofVec3f(x - w / 2, y + h / 2, z));
		mesh.addNormal(ofVec3f(0, 0, 1));
		mesh.addColor(color);

		mesh.addVertex(ofVec3f(x + w / 2, y + h / 2, z));
		mesh.addNormal(ofVec3f(0, 0, 1));
		mesh.addColor(color);

		mesh.draw();
	}
	ofPopStyle();
}

void PianoKey::drawHistory(uint64_t currentTime, uint8_t channel, const noteHistory_t & history, std::deque<channelHistory_t> events) {
	const uint8_t USE_CC = 64;
	const bool DECAY = true;

	int noteInOctave = noteNumber % 12;
	float velocityRatio = pow(history.velocity / 127.f, 0.5f);
	const ofColor BASE_COLOR(255, 64, 0);
	const ofColor PEDAL_COLOR(128, 128, 128);

	auto mapping = std::find_if(
		percussionMappings.begin(),
		percussionMappings.end(),
		[&](const percussionMapping_t & mapping) {
			return mapping.channel == channel && noteNumber == mapping.pitch;
		});
	const float finalPosX = mapping != percussionMappings.end()
		? (calculatePosition(mapping->mapEndPitch + 1) + calculatePosition(mapping->mapStartPitch)) / 2
		: rootPosX;
	const float w = mapping != percussionMappings.end()
		? calculatePosition(mapping->mapEndPitch + 1) - calculatePosition(mapping->mapStartPitch)
		: KEY_ROOT_WIDTHS[noteInOctave];

	if (USE_CC) {
		float x1 = finalPosX - w / 2;
		float x2 = finalPosX + w / 2;
		float z = 1.0;
		float pedalZ = 0.0;

		uint64_t tStart = std::max((int64_t)history.onTime, (int64_t)currentTime - 5'000'000);
		uint64_t tFinal = history.offTime ? history.offTime : currentTime;
		uint64_t tPedalFinal = history.pedalOffTime ? history.pedalOffTime : currentTime;

		// find the last CC event before tStart, and position nextEvent at the next CC event at/after tStart
		auto advanceToCC = [&](std::deque<channelHistory_t>::iterator it) {
			while (it != events.end() && !(it->status == MIDI_CONTROL_CHANGE && it->control == USE_CC)) {
				++it;
			}
			return it;
		};

		std::deque<channelHistory_t>::iterator nextEvent = events.begin();
		std::deque<channelHistory_t>::iterator curEvent = events.end();
		while (nextEvent != events.end() && nextEvent->timestamp < tStart) {
			if (nextEvent->status == MIDI_CONTROL_CHANGE && nextEvent->control == USE_CC) {
				curEvent = nextEvent;
			}
			++nextEvent;
		}
		nextEvent = advanceToCC(nextEvent);

		int8_t ccValue = curEvent != events.end()
			? curEvent->value
			: nextEvent != events.end()
			? nextEvent->value
			: 127;
		uint64_t ccEventTime = curEvent != events.end()
			? curEvent->timestamp
			: nextEvent != events.end()
			? nextEvent->timestamp
			: 0;

		while (tStart < tFinal) {
			int8_t nextCcValue = nextEvent != events.end() ? nextEvent->value : ccValue; // used for transition
			uint64_t nextCcEventTime = nextEvent != events.end() ? nextEvent->timestamp : 0; // used for transition
			uint64_t tEnd = nextCcEventTime ? std::min(nextCcEventTime, tFinal) : tFinal;
			bool needIterateFlag = nextEvent != events.end() && tFinal >= nextCcEventTime;

			uint64_t tLength = tEnd - tStart;

			const float top = (currentTime - tEnd) * speed;
			const float bottom = (currentTime - tStart) * speed;

			const float bottomRatio = ccValue / 127.f;
			const float topRatio = ccValue / 127.f;

			ofColor bottomColor = BASE_COLOR;
			bottomColor.a = 255 * bottomRatio;

			ofColor topColor = BASE_COLOR;
			topColor.a = 255 * topRatio;

			ofPushStyle();
			ofEnableAlphaBlending();
			{
				ofMesh mesh;
				mesh.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);

				mesh.addVertex(ofVec3f(x1, bottom, z));
				mesh.addColor(bottomColor);
				mesh.addVertex(ofVec3f(x2, bottom, z));
				mesh.addColor(bottomColor);

				mesh.addVertex(ofVec3f(x1, top, z));
				mesh.addColor(topColor);
				mesh.addVertex(ofVec3f(x2, top, z));
				mesh.addColor(topColor);

				mesh.draw();
			}
			ofDisableAlphaBlending();
			ofPopStyle();

			tStart = tEnd;
			if (needIterateFlag) {
				ccValue = nextCcValue;
				ccEventTime = nextCcEventTime;
				nextEvent = advanceToCC(++nextEvent);
			} else {
				break;
			}
		}

		tStart = std::max((int64_t)tFinal, (int64_t)currentTime - 5'000'000);

		while (tStart < tPedalFinal) {
			int8_t nextCcValue = nextEvent != events.end() ? nextEvent->value : ccValue; // used for transition
			uint64_t nextCcEventTime = nextEvent != events.end() ? nextEvent->timestamp : 0; // used for transition
			uint64_t tEnd = nextCcEventTime ? std::min(nextCcEventTime, tPedalFinal) : tPedalFinal;
			bool needIterateFlag = nextEvent != events.end() && tPedalFinal >= nextCcEventTime;

			uint64_t tLength = tEnd - tStart;

			const float top = (currentTime - tEnd) * speed;
			const float bottom = (currentTime - tStart) * speed;

			const float bottomRatio = ccValue / 127.f;
			const float topRatio = ccValue / 127.f;

			ofColor bottomColor = PEDAL_COLOR;
			bottomColor.a = 255 * bottomRatio;

			ofColor topColor = PEDAL_COLOR;
			topColor.a = 255 * topRatio;

			ofPushStyle();
			ofEnableAlphaBlending();
			{
				ofMesh mesh;
				mesh.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);

				mesh.addVertex(ofVec3f(x1, bottom, z));
				mesh.addColor(bottomColor);
				mesh.addVertex(ofVec3f(x2, bottom, z));
				mesh.addColor(bottomColor);

				mesh.addVertex(ofVec3f(x1, top, z));
				mesh.addColor(topColor);
				mesh.addVertex(ofVec3f(x2, top, z));
				mesh.addColor(topColor);

				mesh.draw();
			}
			ofDisableAlphaBlending();
			ofPopStyle();

			tStart = tEnd;
			if (needIterateFlag) {
				ccValue = nextCcValue;
				ccEventTime = nextCcEventTime;
				nextEvent = advanceToCC(++nextEvent);
			} else {
				break;
			}
		}
	} else if (DECAY) {
		const int TIME_SEGMENT = 100000; // us
		const float DECAY_RATE = 0.95f; // decay ratio for every time segment

		float x1 = finalPosX - w / 2;
		float x2 = finalPosX + w / 2;
		float z = 1.0;
		float pedalZ = 0.0;

		uint64_t tStart = std::max((int64_t)history.onTime, (int64_t)currentTime - 5'000'000);
		uint64_t tFinal = history.offTime ? history.offTime : currentTime;
		uint64_t tPedalFinal = history.pedalOffTime ? history.pedalOffTime : currentTime;

		for (uint64_t time = tStart; time < tFinal; time += TIME_SEGMENT) {
			uint64_t tEnd = std::min(time + TIME_SEGMENT, tFinal);
			uint64_t tLength = tEnd - tStart;

			const float top = (currentTime - tEnd) * speed;
			const float bottom = (currentTime - tStart) * speed;

			const float bottomRatio = pow(DECAY_RATE, (tStart - history.onTime) / TIME_SEGMENT);
			const float topRatio = pow(DECAY_RATE, (tEnd - history.onTime) / TIME_SEGMENT);

			ofColor bottomColor = BASE_COLOR;
			bottomColor.a = 255 * bottomRatio * velocityRatio;

			ofColor topColor = BASE_COLOR;
			topColor.a = 255 * topRatio * velocityRatio;

			ofPushStyle();
			ofEnableAlphaBlending();
			{
				ofMesh mesh;
				mesh.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);

				mesh.addVertex(ofVec3f(x1, bottom, z));
				mesh.addColor(bottomColor);
				mesh.addVertex(ofVec3f(x2, bottom, z));
				mesh.addColor(bottomColor);

				mesh.addVertex(ofVec3f(x1, top, z));
				mesh.addColor(topColor);
				mesh.addVertex(ofVec3f(x2, top, z));
				mesh.addColor(topColor);

				mesh.draw();
			}
			ofDisableAlphaBlending();
			ofPopStyle();

			tStart = time;
		}

		tStart = std::max((int64_t)tFinal, (int64_t)currentTime - 5'000'000);

		for (uint64_t time = tStart; time < tPedalFinal; time += TIME_SEGMENT) {
			uint64_t tEnd = std::min(time + TIME_SEGMENT, tPedalFinal);
			uint64_t tLength = tEnd - tStart;

			const float top = (currentTime - tEnd) * speed;
			const float bottom = (currentTime - tStart) * speed;

			const float bottomRatio = pow(DECAY_RATE, (tStart - history.onTime) / TIME_SEGMENT);
			const float topRatio = pow(DECAY_RATE, (tEnd - history.onTime) / TIME_SEGMENT);

			ofColor bottomColor = PEDAL_COLOR;
			bottomColor.a = 255 * bottomRatio * velocityRatio;

			ofColor topColor = PEDAL_COLOR;
			topColor.a = 255 * topRatio * velocityRatio;

			ofPushStyle();
			ofEnableAlphaBlending();
			{
				ofMesh mesh;
				mesh.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);

				mesh.addVertex(ofVec3f(x1, bottom, pedalZ));
				mesh.addColor(bottomColor);
				mesh.addVertex(ofVec3f(x2, bottom, pedalZ));
				mesh.addColor(bottomColor);

				mesh.addVertex(ofVec3f(x1, top, pedalZ));
				mesh.addColor(topColor);
				mesh.addVertex(ofVec3f(x2, top, pedalZ));
				mesh.addColor(topColor);

				mesh.draw();
			}
			ofDisableAlphaBlending();
			ofPopStyle();

			tStart = time;
		}
	} else {
		const float top = (currentTime - history.onTime) * speed;
		const float bottom = history.offTime ? (currentTime - history.offTime) * speed : 0;
		const float pedalBottom = history.pedalOffTime ? (currentTime - history.pedalOffTime) * speed : 0;
		const float h = top - bottom;
		const float pedalH = bottom - pedalBottom;

		ofPushStyle();
		ofSetColor(BASE_COLOR);
		ofDrawBox(finalPosX, (top + bottom) / 2, 0, w, h, 1);
		ofPopStyle();

		ofPushStyle();
		ofSetColor(PEDAL_COLOR); // Gray when pedal is down
		ofDrawBox(finalPosX, (bottom + pedalBottom) / 2, 0, w, pedalH, 1);
		ofPopStyle();
	}
}

void PianoKey::setActive(bool active) {
	isActive = active;
}
