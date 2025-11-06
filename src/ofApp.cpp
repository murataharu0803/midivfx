#include <algorithm>

#include "ofApp.h"

const int MAX_HISTORY_SIZE = 1024;

void ofApp::setup() {
	ofSetFrameRate(60);
	ofSetVerticalSync(true);
	ofBackground(20);

	// Setup camera
	camera.setPosition(0, 500, 1800); // Initial position
	camera.setTarget(ofVec3f(0, 500, 0)); // Look at origin

	// Setup lighting
	ofEnableLighting();
	ofEnableSeparateSpecularLight();

	ofSetGlobalAmbientColor(ofColor(128, 128, 128));

	directionalLight.setDirectional();
	directionalLight.setPosition(0, -800, -1000);
	directionalLight.setDiffuseColor(ofColor(255, 255, 255));
	directionalLight.setSpecularColor(ofColor(255, 255, 255));
	directionalLight.lookAt(ofVec3f(0, 0, 0));

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

	for (auto & noteHistVector : noteHistories) {
		while (noteHistVector.size() > 0) {
			uint64_t pedalOffTime = noteHistVector.front().pedalOffTime;
			if (pedalOffTime > 0 && (int64_t)pedalOffTime < (int64_t)currentTime - 5'000'000) {
				noteHistVector.pop_front(); // remove old history
			} else {
				break;
			}
		}
	}
}

void ofApp::draw() {
	camera.begin();
	ofEnableDepthTest();

	// Enable lights
	directionalLight.enable();
	// pointLight.enable();

	// Draw piano keys on top
	pianoKeys.draw(currentTime);

	// Disable lights before 2D drawing
	// pointLight.disable();
	directionalLight.disable();

	ofDisableDepthTest();
	camera.end();

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

	// channel history
	auto histories = channelHistories[event.channel - 1];
	if (histories.size() > MAX_HISTORY_SIZE - 1) {
		histories.pop_front();
	}
	histories.push_back({
		currentTime,
		status,
		static_cast<uint8_t>(event.pitch),
		static_cast<uint8_t>(event.control),
		static_cast<uint8_t>(event.value),
	});

	// pedal status
	bool oldPedalDown = pedalDown;
	if (status == MIDI_CONTROL_CHANGE && event.control == 64) {
		pedalDown = (event.value >= 64);
	}

	// note status
	auto & noteStatus = keyStatuses[event.channel - 1][event.pitch];
	if (status == MIDI_NOTE_ON && event.velocity > 0) {
		noteStatus.isOn = true;
		noteStatus.velocity = event.velocity;
	} else if (status == MIDI_NOTE_OFF || (status == MIDI_NOTE_ON && event.velocity == 0)) {
		noteStatus.isOn = false;
		noteStatus.velocity = 0;
		if (!pedalDown) {
			noteStatus.isPedalOn = false;
		}
	} else if (status == MIDI_POLY_AFTERTOUCH) {
		noteStatus.velocity = event.value;
	} else if (oldPedalDown && !pedalDown) { // Pedal released
		for (auto & noteStatus : keyStatuses[event.channel - 1]) {
			if (!noteStatus.isOn) {
				noteStatus.isPedalOn = false;
			}
		}
	}

	// note history
	auto & noteHistVector = noteHistories[event.channel - 1];
	if (status == MIDI_NOTE_ON && event.velocity > 0) {
		// check if not already on
		auto noteHistory = std::find_if(
			noteHistVector.begin(),
			noteHistVector.end(),
			[&](const noteHistory_t & nh) {
				return nh.pitch == event.pitch && nh.offTime == 0;
			});
		if (noteHistory == noteHistVector.end()) {
			// find one that is still pedaled
			auto pedalNoteHistory = std::find_if(
				noteHistVector.begin(),
				noteHistVector.end(),
				[&](const noteHistory_t & nh) {
					return nh.pitch == event.pitch && nh.offTime && nh.pedalOffTime == 0;
				});
			if (pedalNoteHistory != noteHistVector.end()) {
				pedalNoteHistory->pedalOffTime = currentTime;
			}
			// first check size limit
			if (noteHistVector.size() > MAX_HISTORY_SIZE - 1) {
				noteHistVector.pop_front();
			}
			// and then create history
			noteHistVector.push_back({
				currentTime,
				0,
				0,
				static_cast<uint8_t>(event.pitch),
				static_cast<uint8_t>(event.velocity),
			});
		}
	} else if (status == MIDI_NOTE_OFF || (status == MIDI_NOTE_ON && event.velocity == 0)) {
		auto noteHistory = std::find_if(
			noteHistVector.begin(),
			noteHistVector.end(),
			[&](const noteHistory_t & nh) {
				return nh.pitch == event.pitch && nh.offTime == 0;
			});
		if (noteHistory != noteHistVector.end()) {
			noteHistory->offTime = currentTime;
			if (!pedalDown) {
				noteHistory->pedalOffTime = currentTime;
			}
		}
	} else if (oldPedalDown && !pedalDown) { // Pedal released
		for (auto & noteHistory : noteHistVector) {
			if (!noteHistory.pedalOffTime && noteHistory.offTime) {
				noteHistory.pedalOffTime = currentTime;
			}
		}
	}
}
