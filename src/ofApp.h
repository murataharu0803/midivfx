#pragma once

#include "ofMain.h"
#include "ofxMidi.h"

#include "MidiFileLoader.h"
#include "MidiProcessor.h"
#include "PianoKeys.h"
#include "VisualizerConfig.h"
#include "midiUtil.h"

using namespace std;

class ofApp : public ofBaseApp, public ofxMidiListener {
public:
	// ofBaseApp
	void setup() override;
	void update() override;
	void draw() override;
	void exit() override;

	void keyPressed(int key) override;
	void keyReleased(int key) override;
	void mouseMoved(int x, int y) override;
	void mouseDragged(int x, int y, int button) override;
	void mousePressed(int x, int y, int button) override;
	void mouseReleased(int x, int y, int button) override;
	void mouseScrolled(int x, int y, float scrollX, float scrollY) override;
	void mouseEntered(int x, int y) override;
	void mouseExited(int x, int y) override;
	void windowResized(int w, int h) override;
	void dragEvent(ofDragInfo dragInfo) override;
	void gotMessage(ofMessage msg) override;

	ofEasyCam camera;

	// Lighting
	ofLight directionalLight;
	// ofLight pointLight;

	// ofxMidiListener
	void newMidiMessage(ofxMidiMessage & event) override;

	// time
	int64_t currentTime = 0;

	// MIDI
	ofxMidiIn midiIn;
	MidiProcessor midiProcessor;

	// visual objects
	PianoKeys pianoKeys = PianoKeys(midiProcessor.channels, beatEvents);

	VisualizerConfig config;

	vector<MidiFileEvent> midiFileEvents;
	vector<BeatEvent> beatEvents;
	size_t playbackHead = 0;
	int64_t playbackStartTime = -5'000'000;
};
