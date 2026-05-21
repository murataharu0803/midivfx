#pragma once

#include <string>
#include <vector>
#include "ofColor.h"
#include "midiUtil.h"

enum class NoteRenderMode { Default, Decay, CC };

struct VisualizerConfig {
	// Playback source
	bool useMidiFile = true;
	std::string midiFilePath = "song.mid"; // place in bin/data/
	int midiInPort = 1;                   // live MIDI input port index

	// Timing
	int64_t dispatchOffset = 0;           // how early before onTime to dispatch events
	int64_t removeOffset = 5'000'000;     // how long after pedalOffTime to keep history (us)

	// Display
	bool reverseMode = false;             // true = notes fall toward piano
	float speed = 0.001f;                 // pixels per microsecond
	NoteRenderMode renderMode = NoteRenderMode::Decay;

	// Colors
	ofColor noteColor = ofColor(255, 64, 0);
	ofColor pedalColor = ofColor(128, 128, 128);

	// Decay mode
	float decayRate = 0.95f;              // alpha multiplier per decayTimeSegment
	int64_t decayTimeSegment = 100'000;   // segment length for decay steps (us)

	// Camera
	float cameraX = 0.f;
	float cameraY = 500.f;
	float cameraZ = 1800.f;
	float cameraTargetY = 500.f;          // camera looks at (0, cameraTargetY, 0)

	// Percussion mappings
	std::vector<percussionMapping_t> percussionMappings = {
		{ 0, 9, 36, 24, 36, false }, // map MIDI ch10 (0-based: 9) C2 across C notes
	};
};
