#pragma once

#include "ofMain.h"
#include "VisualizerConfig.h"
#include "midiUtil.h"
#include <vector>

class VideoExporter {
public:
	void setup(const VisualizerConfig & cfg, const std::vector<MidiFileEvent> & events);

	// Call at the start of draw() to redirect rendering into the FBO
	void begin();

	// Call at the end of draw() (after camera.end()) to save the frame.
	// Returns true on the frame that triggers ffmpeg + exit.
	bool end();

	// Advance synthetic time by one frame and return the new elapsed µs
	// (add playbackStartTime yourself, same as before).
	int64_t advanceAndGetElapsedUs();

	bool isActive() const { return active; }

private:
	void finish();

	const VisualizerConfig * config = nullptr;
	ofFbo fbo;
	ofPixels pixels;
	int frameIndex = 0;
	int64_t syntheticElapsedUs = 0;
	int64_t endTimeUs = 0;
	bool active = false;
};
