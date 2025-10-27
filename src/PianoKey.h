#pragma once

#include "ofMain.h"

#include "midiUtil.h"

class PianoKey {
public:
	static float getKeysWidth(int begin, int end);

	int noteNumber; // 0-127
	bool isBlackKey;
	bool isActive;

	float posX, rootPosX; // Position
	float width, height;
	ofColor color;

	PianoKey(int note);

	void draw();
	void drawHistory(const noteHistory_t & history, uint64_t currentTime);
	void setActive(bool active);

private:
	void calculatePosition();
	void calculateRootCenter();
	void calculateDimensions();
};
