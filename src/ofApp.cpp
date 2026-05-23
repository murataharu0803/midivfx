#include "ofApp.h"
#include "ConfigLoader.h"
#include "PianoKey.h"

void ofApp::setup() {
	ofSetFrameRate(60);
	ofSetVerticalSync(true);
	ofBackground(20);

	// Setup camera
	if (config.display.horizontal) {
		camera.setPosition(config.camera.targetTimeAxis, config.camera.pitchAxis, config.camera.zAxis);
		camera.setTarget(ofVec3f(config.camera.targetTimeAxis, config.camera.targetPitchAxis, 0));
	} else {
		camera.setPosition(config.camera.pitchAxis, config.camera.timeAxis, config.camera.zAxis);
		camera.setTarget(ofVec3f(config.camera.targetPitchAxis, config.camera.targetTimeAxis, 0));
	}

	// Setup lighting
	ofEnableLighting();
	ofEnableSeparateSpecularLight();

	ofSetGlobalAmbientColor(ofColor(128, 128, 128));

	directionalLight.setDirectional();
	directionalLight.setPosition(0, -800, -1000);
	directionalLight.setDiffuseColor(ofColor(255, 255, 255));
	directionalLight.setSpecularColor(ofColor(255, 255, 255));
	directionalLight.lookAt(ofVec3f(0, 0, 0));

	pianoKeys.setup(config);

	if (config.playback.mode != PlaybackMode::Live) {
		std::string midiPath = ofFilePath::isAbsolute(config.playback.midiFilePath)
			? config.playback.midiFilePath
			: ofFilePath::getCurrentExeDir() + config.playback.midiFilePath;
		auto loaded = MidiFileLoader::load(midiPath);
		midiFileEvents = std::move(loaded.events);
		beatEvents = std::move(loaded.beatEvents);
		midiProcessor.initTracks(loaded.trackCount);
	} else {
		midiProcessor.initTracks(1); // one track per port; extend here for multi-port
		midiIn.listInPorts();
		midiIn.openPort(config.playback.midiInPort);
		midiIn.addListener(this);
	}

	exporter.setup(config, midiFileEvents);
}

void ofApp::update() {
	if (exporter.isActive()) {
		currentTime = exporter.advanceAndGetElapsedUs() + playbackStartTime;
	} else {
		currentTime = ofGetElapsedTimeMicros() + playbackStartTime;
	}

	if (config.playback.mode != PlaybackMode::Live) {
		int64_t lookaheadTime = currentTime + config.timing.dispatchOffset;
		while (playbackHead < midiFileEvents.size() && midiFileEvents[playbackHead].timeUs <= lookaheadTime) {
			auto & e = midiFileEvents[playbackHead++];
			ofxMidiMessage msg;
			msg.status = (MidiStatus)(e.status);
			msg.channel = e.channel + 1; // processMidiMessage expects 1-based
			msg.pitch = e.data1;
			msg.control = e.data1;
			msg.velocity = e.data2;
			msg.value = e.data2;
			midiProcessor.processMidiMessage(msg, e.track, e.timeUs);
		}
	}

	// Update piano key active state
	auto activeKeys = midiProcessor.getActiveKeys();
	for (int i = 0; i < 128; ++i) {
		pianoKeys.keys[i].setActive(activeKeys[i]);
	}

	midiProcessor.trimHistory(currentTime, config.timing.removeOffset);
}

void ofApp::draw() {
	if (exporter.isActive()) exporter.begin();

	camera.begin();
	ofEnableDepthTest();

	directionalLight.enable();
	// pointLight.enable();

	pianoKeys.draw(currentTime, config);

	// pointLight.disable();
	directionalLight.disable();

	ofDisableDepthTest();
	camera.end();

	// 2D UI on top
	if (!exporter.isActive()) {
		ofSetColor(255);
		ofDrawBitmapString("FPS: " + ofToString(ofGetFrameRate(), 0), 20, 20);
		ofDrawBitmapString("Mouse drag to rotate, scroll to zoom", 20, 40);
	}

	if (exporter.isActive()) exporter.end();
}

void ofApp::newMidiMessage(ofxMidiMessage & event) {
	midiProcessor.processMidiMessage(event, 0, currentTime); // live MIDI always maps to track 0
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
