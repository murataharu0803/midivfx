#pragma once

#include "ofMain.h"

class PianoKey {
public:
	static float getKeysWidth(int begin, int end);

	int noteNumber; // 0-127
	bool isBlackKey;
	bool isActive;

	float posX; // Position
	float width, height;
	ofColor color;

	PianoKey(int note);

	void draw();
	void setActive(bool active);

private:
	void calculatePosition();
	void calculateDimensions();
};
