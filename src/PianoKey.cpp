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

PianoKey::PianoKey(int note)
	: noteNumber(note)
	, isActive(false) {
	int noteInOctave = note % 12;
	isBlackKey = BLACK_KEY_PATTERN[noteInOctave];

	calculatePosition();
	calculateRootCenter();
	calculateDimensions();

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

void PianoKey::calculatePosition() {
	int octave = noteNumber / 12;
	int noteInOctave = noteNumber % 12;
	posX = KEY_END_OFFSETS[noteInOctave] + octave * OCTAVE_WIDTH;
}

void PianoKey::calculateRootCenter() {
	int octave = noteNumber / 12;
	int noteInOctave = noteNumber % 12;
	rootPosX = NOTE_CENTER_OFFSETS[noteInOctave] + octave * OCTAVE_WIDTH;
}

void PianoKey::calculateDimensions() {
	int noteInOctave = noteNumber % 12;
	width = KEY_END_WIDTHS[noteInOctave];
	height = isBlackKey ? BLACK_KEY_HEIGHT : WHITE_KEY_HEIGHT;
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

void PianoKey::drawHistory(const noteHistory_t & history, uint64_t currentTime) {
	const bool DECAY = true;

	int noteInOctave = noteNumber % 12;
	const float w = KEY_ROOT_WIDTHS[noteInOctave];
	float velocityRatio = pow(history.velocity / 127.f, 0.5f);
	const ofColor BASE_COLOR(255, 64, 0);
	const ofColor PEDAL_COLOR(128, 128, 128);

	if (DECAY) {
		const int TIME_SEGMENT = 100000; // us
		const float DECAY_RATE = 0.95f; // decay ratio for every time segment

		float x1 = rootPosX - w / 2;
		float x2 = rootPosX + w / 2;
		float z = 1.0;
		float pedalZ = 0.0;

		uint64_t tStart = history.onTime;
		uint64_t tFinal = history.offTime ? history.offTime : currentTime;
		uint64_t tPedalFinal = history.pedalOffTime ? history.pedalOffTime : currentTime;

		for (uint64_t time = history.onTime; time < tFinal; time += TIME_SEGMENT) {
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

		tStart = tFinal;

		for (uint64_t time = tFinal; time < tPedalFinal; time += TIME_SEGMENT) {
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
		ofDrawBox(rootPosX, (top + bottom) / 2, 0, w, h, 1);
		ofPopStyle();

		ofPushStyle();
		ofSetColor(PEDAL_COLOR); // Gray when pedal is down
		ofDrawBox(rootPosX, (bottom + pedalBottom) / 2, 0, w, pedalH, 1);
		ofPopStyle();
	}
}

void PianoKey::setActive(bool active) {
	isActive = active;
}
