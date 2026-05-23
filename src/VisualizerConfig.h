#pragma once

#include "midiUtil.h"
#include "ofColor.h"
#include <string>
#include <vector>

enum class NoteRenderMode { Default,
	Decay,
	CC };

struct VisualizerConfig {
	// Playback source
	bool useMidiFile = true;
	std::string midiFilePath = "song.mid"; // place in bin/data/
	int midiInPort = 1; // live MIDI input port index

	// Timing
	int64_t dispatchOffset = 5'000'000; // how early before onTime to dispatch events
	int64_t removeOffset = 5'000'000; // how long after pedalOffTime to keep history (us)

	// Display
	bool showPiano = false;
	bool isAverageWidth = true;
	bool reverseMode = true;
	bool horizontalMode = true;
	float speed = 0.001f; // pixels per microsecond
	NoteRenderMode renderMode = NoteRenderMode::Decay;

	// Colors
	ofColor noteColor = ofColor(255, 160, 0);
	ofColor pedalColor = ofColor(160, 160, 160);

	// Decay mode
	float decayRate = 0.95f; // alpha multiplier per decayTimeSegment
	int64_t decayTimeSegment = 100'000; // segment length for decay steps (us)

	// Camera
	float cameraAlongPitchAxis = 0.f;
	float cameraAlongTimeAxis = 500.f;
	float cameraZ = horizontalMode ? 3200.f : 1800.f;
	float cameraTargetAlongPitchAxis = 0.f;
	float cameraTargetAlongTimeAxis = 500.f;

	// Percussion mappings
	std::vector<percussionMapping_t> percussionMappings = {
		{ 0, 9, 36, 24, 36, false }, // map MIDI ch10 (0-based: 9) C2 across C notes
	};

	// Export
	bool exportMode = false;
	std::string exportOutputPath = "output.mp4";
	int exportFps = 60;
	int64_t exportEndPaddingUs = 3'000'000; // extra time after last note (us)
};
