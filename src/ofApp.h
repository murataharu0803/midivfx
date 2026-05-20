#pragma once

#include "ofMain.h"

#include "ofxMidi.h"
#include "ofxMidifile.h"

#include "PianoKeys.h"
#include "midiUtil.h"
#include "ChannelState.h"

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
	// [track][channel(0-based)] — track count set at setup; 16 channels fixed by MIDI spec
	vector<array<ChannelState, 16>> channels;

	// visual objects
	PianoKeys pianoKeys = PianoKeys(channels);

	// MIDI file playback
	static constexpr bool useMidiFile = true;
	static constexpr const char * midiFilePath = "song.mid"; // place in bin/data/
	static constexpr int64_t dispatchOffset = 0; // how early before onTime to create history entry
	static constexpr int64_t removeOffset = 5'000'000; // how long after offTime or PedalOffTime to keep entry
	static constexpr bool reverseMode = false; // true = notes fall downward toward piano

	vector<MidiFileEvent> midiFileEvents;
	vector<BeatEvent> beatEvents;
	size_t playbackHead = 0;
	int64_t playbackStartTime = -5'000'000;

private:
	void processMidiMessage(ofxMidiMessage & event, uint8_t track, int64_t timestamp);
	void initTracks(int count);
};
