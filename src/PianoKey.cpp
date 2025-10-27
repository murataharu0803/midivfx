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
	ofPushStyle();

	// Draw key body
	if (isActive) {
		ofSetColor(255, 150, 0); // Orange when active
	} else {
		ofSetColor(color);
	}

	ofDrawBox(
		posX + width / 2,
		-height / 2,
		isBlackKey ? 1 : 0,
		isBlackKey ? width : width - padding,
		height,
		1);

	ofPopStyle();
}

void PianoKey::drawHistory(const noteHistory_t & history, uint64_t currentTime) {
	ofPushStyle();

	ofSetColor(255, 150, 0); // Orange when active
	int noteInOctave = noteNumber % 12;
	const float top = (currentTime - history.onTime) * speed;
	const float bottom = history.offTime ? (currentTime - history.offTime) * speed : 0;
	const float w = KEY_ROOT_WIDTHS[noteInOctave];
	const float h = top - bottom;

	ofDrawBox(rootPosX, (top + bottom) / 2, 0, w, h, 1);

	ofPopStyle();
}

void PianoKey::setActive(bool active) {
	isActive = active;
}
