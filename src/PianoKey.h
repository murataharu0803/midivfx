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
	void drawHistory(uint64_t currentTime, const noteHistory_t & history, std::deque<channelHistory_t> events);
	void setActive(bool active);

private:
	void calculatePosition();
	void calculateRootCenter();
	void calculateDimensions();
};
