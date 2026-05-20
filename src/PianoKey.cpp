#include "PianoKey.h"

// layout constants
const float SCALE = 2.f;
const float OCTAVE_WIDTH = 164 * SCALE;
const float WHITE_KEY_WIDTH = 23 * SCALE;
const float BLACK_KEY_WIDTH = 14 * SCALE;
const float WHITE_KEY_HEIGHT = 150 * SCALE;
const float BLACK_KEY_HEIGHT = 100 * SCALE;

const float padding = 2.f; // padding between keys

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

void PianoKey::drawHistory(
	int64_t currentTime,
	uint8_t track,
	uint8_t channel,
	const noteHistory_t & history,
	const std::deque<channelHistory_t> & events,
	bool reverseMode,
	int64_t dispatchOffset,
	int64_t removeOffset,
	NoteRenderMode mode) {

	int noteInOctave = noteNumber % 12;

	// Resolve percussion mapping to determine display position and width
	auto mapping = std::find_if(
		percussionMappings.begin(),
		percussionMappings.end(),
		[&](const percussionMapping_t & m) {
			return m.track == track && m.channel == channel && noteNumber == m.pitch;
		});
	const float finalPosX = mapping != percussionMappings.end()
		? (calculatePosition(mapping->mapEndPitch + 1) + calculatePosition(mapping->mapStartPitch)) / 2
		: rootPosX;
	const float w = mapping != percussionMappings.end()
		? calculatePosition(mapping->mapEndPitch + 1) - calculatePosition(mapping->mapStartPitch)
		: KEY_ROOT_WIDTHS[noteInOctave];

	NoteHistoryRenderer::draw(currentTime, track, channel, finalPosX, w,
		history, events, reverseMode, dispatchOffset, removeOffset, mode);
}

void PianoKey::setActive(bool active) {
	isActive = active;
}
