#include "ofApp.h"

const int MAX_HISTORY_SIZE = 1024;

void ofApp::setup() {
	ofSetFrameRate(60);
	ofSetVerticalSync(true);
	ofBackground(20);

	// Setup camera
	camera.setPosition(0, 500, 1800); // Initial position
	camera.setTarget(ofVec3f(0, 500, 0)); // Look at origin

	pianoKeys.setup();

	// List MIDI ports
	midiIn.listInPorts();

	// Open first port (or choose specific one)
	midiIn.openPort(1);
	midiIn.addListener(this);
}

void ofApp::update() {
	currentTime = ofGetElapsedTimeMicros();

	// Update your note positions, animations, etc.
	for (int i = 0; i < 128; ++i) {
		pianoKeys.keys[i].setActive(keyStatuses[0][i].isOn);
	}
}

void ofApp::draw() {
	camera.begin();
	ofEnableDepthTest();

	pianoKeys.draw();

	ofDisableDepthTest();
	camera.end(); // ← End 3D camera

	// 2D UI on top
	ofSetColor(255);
	ofDrawBitmapString("FPS: " + ofToString(ofGetFrameRate(), 0), 20, 20);
	ofDrawBitmapString("Mouse drag to rotate, scroll to zoom", 20, 40);
}

void ofApp::exit() { }

void ofApp::keyPressed(int key) { }

void ofApp::keyReleased(int key) { }

void ofApp::mouseMoved(int x, int y) { }

void ofApp::mouseDragged(int x, int y, int button) { }

void ofApp::mousePressed(int x, int y, int button) { }

void ofApp::mouseReleased(int x, int y, int button) { }

void ofApp::mouseScrolled(int x, int y, float scrollX, float scrollY) { }

void ofApp::mouseEntered(int x, int y) { }

void ofApp::mouseExited(int x, int y) { }

void ofApp::windowResized(int w, int h) { }

void ofApp::gotMessage(ofMessage msg) { }

void ofApp::dragEvent(ofDragInfo dragInfo) { }

void ofApp::newMidiMessage(ofxMidiMessage & event) {
	ofLogNotice() << event.toString();

	MidiStatus status = event.status;

	auto histories = channelHistories[event.channel - 1];
	histories.push({
		currentTime,
		status,
		static_cast<uint8_t>(event.pitch),
		static_cast<uint8_t>(event.control),
		static_cast<uint8_t>(event.value),
	});
	while (histories.size() > MAX_HISTORY_SIZE) {
		histories.pop();
	}

	keyStatus_t & noteStatus = keyStatuses[event.channel - 1][event.pitch];
	if (status == MIDI_NOTE_ON && event.velocity > 0) {
		noteStatus.isOn = true;
		noteStatus.velocity = event.velocity;
	} else if (status == MIDI_NOTE_OFF || (status == MIDI_NOTE_ON && event.velocity == 0)) {
		noteStatus.isOn = false;
	} else if (status == MIDI_POLY_AFTERTOUCH) {
		noteStatus.velocity = event.value;
	}
}
