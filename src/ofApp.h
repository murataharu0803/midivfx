#pragma once

#include "ofMain.h"
#include "ofxGui.h"
#include "ofxMidi.h"
#include "ofxPostProcessing.h"

#include "PianoKeys.h"
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
	// ofxMidiListener
	void newMidiMessage(ofxMidiMessage & event);

	// time
	uint64_t currentTime = 0;

	// MIDI
	ofxMidiIn midiIn;
	array<array<keyStatus_t, 128>, 256> keyStatuses; // [channel][pitch]
	array<deque<noteHistory_t>, 256> noteHistories; // [channel]
	array<deque<channelHistory_t>, 256> channelHistories; // [channel]

	// visual objects
	PianoKeys pianoKeys = PianoKeys(noteHistories);

	// Visual params
	float hue = 0.5f;
	float brightness = 0.5f;
	float rotationSpeed = 0.01f;

	// Post-processing
	ofxPostProcessing post;

	// GUI
	ofxPanel gui;
	ofParameter<float> bloomIntensity;
	ofParameter<float> bloomRadius;
};
