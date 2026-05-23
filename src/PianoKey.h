#pragma once

#include "ofMain.h"

#include "NoteHistoryRenderer.h"
#include "VisualizerConfig.h"
#include "midiUtil.h"


class PianoKey {
public:
	static float getKeysWidth(int begin, int end);

	uint8_t noteNumber; // 0-127
	bool isBlackKey;
	bool isActive;

	float posX, rootPosX; // Position
	float width, height;
	ofColor color;

	PianoKey(uint8_t note, const VisualizerConfig & config);

	void draw();
	void drawHistory(
		int64_t currentTime,
		uint8_t track,
		uint8_t channel,
		const noteHistory_t & history,
		const std::deque<channelHistory_t> & events,
		const std::deque<channelHistory_t> & pedalEvents,
		const Style & style,
		const VisualizerConfig & config);
	void setActive(bool active);

private:
	static float calculatePosition(uint8_t noteNumber);
	static float calculateRootCenter(uint8_t noteNumber);
	static float calculateWidth(uint8_t noteNumber);
	static float calculateHeight(uint8_t noteNumber);
};
