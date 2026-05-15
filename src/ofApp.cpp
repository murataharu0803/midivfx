#include <algorithm>

#include "ofApp.h"

const int MAX_HISTORY_SIZE = 1024;

void ofApp::initTracks(int count) {
	keyStatuses.resize(count);
	noteHistories.resize(count);
	channelHistories.resize(count);
	pedalDown.resize(count);
	for (auto & arr : pedalDown)
		arr.fill(false);
}

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

	if (useMidiFile) {
		ofxMidifile mf;
		if (mf.load(midiFilePath)) {
			smf::MidiFile & smf = mf.get();
			ofLogNotice() << "Tracks: " << smf.getTrackCount();
			smf.doTimeAnalysis();

			initTracks(smf.getTrackCount());

			for (int t = 0; t < smf.getTrackCount(); ++t) {
				for (int i = 0; i < smf[t].size(); ++i) {
					smf::MidiEvent & ev = smf[t][i];
					if (!ev.isNoteOn() && !ev.isNoteOff() && !ev.isController()) continue;
					midiFileEvents.push_back({
						(uint64_t)(ev.seconds * 1'000'000),
						(uint8_t)(ev[0] & 0xF0),
						(uint8_t)t,
						(uint8_t)ev.getChannel(), // 0-based
						(uint8_t)ev.getP1(),
						(uint8_t)ev.getP2(),
					});
				}
			}
			std::sort(midiFileEvents.begin(), midiFileEvents.end(),
				[](const MidiFileEvent & a, const MidiFileEvent & b) { return a.timeUs < b.timeUs; });
			playbackStartTime = currentTime;
		}
	} else {
		initTracks(1); // one track per port; extend here for multi-port

		// List MIDI ports
		midiIn.listInPorts();

		// Open first port (or choose specific one)
		midiIn.openPort(1);
		midiIn.addListener(this);
	}
}

void ofApp::update() {
	currentTime = ofGetElapsedTimeMicros();

	if (useMidiFile) {
		uint64_t playbackTime = currentTime - playbackStartTime;
		while (playbackHead < midiFileEvents.size() && midiFileEvents[playbackHead].timeUs <= playbackTime) {
			auto & e = midiFileEvents[playbackHead++];
			ofxMidiMessage msg;
			msg.status = (MidiStatus)(e.status);
			msg.channel = e.channel + 1; // processMidiMessage expects 1-based
			msg.pitch = e.data1;
			msg.velocity = e.data2;
			msg.value = e.data2;
			processMidiMessage(msg, e.track);
		}
	}

	// Update piano key active state — any track/channel activates the key
	for (int i = 0; i < 128; ++i) {
		bool active = false;
		for (auto & trackStatuses : keyStatuses) {
			for (auto & chStatuses : trackStatuses) {
				if (chStatuses[i].isOn) {
					active = true;
					break;
				}
			}
			if (active) break;
		}
		pianoKeys.keys[i].setActive(active);
	}

	// Remove old note history
	for (auto & trackHistories : noteHistories) {
		for (auto & noteHistVector : trackHistories) {
			while (!noteHistVector.empty()) {
				uint64_t pedalOffTime = noteHistVector.front().pedalOffTime;
				if (pedalOffTime > 0 && (int64_t)pedalOffTime < (int64_t)currentTime - 5'000'000) {
					noteHistVector.pop_front();
				} else {
					break;
				}
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
	processMidiMessage(event, 0); // live MIDI always maps to track 0
}

void ofApp::processMidiMessage(ofxMidiMessage & event, uint8_t track) {
	if (track >= keyStatuses.size()) {
		ofLogWarning() << "processMidiMessage: track " << track << " out of range";
		return;
	}

	// ofLogNotice() << event.toString();

	const uint8_t ch = event.channel - 1; // convert to 0-based
	MidiStatus status = event.status;

	// channel history
	auto & histories = channelHistories[track][ch];
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
	bool & channelPedalDown = pedalDown[track][ch];
	bool oldChannelPedalDown = channelPedalDown;
	if (status == MIDI_CONTROL_CHANGE && event.control == 64) {
		channelPedalDown = (event.value >= 64);
	}

	// note status
	auto & noteStatus = keyStatuses[track][ch][event.pitch];
	if (status == MIDI_NOTE_ON && event.velocity > 0) {
		noteStatus.isOn = true;
		noteStatus.velocity = event.velocity;
	} else if (status == MIDI_NOTE_OFF || (status == MIDI_NOTE_ON && event.velocity == 0)) {
		noteStatus.isOn = false;
		noteStatus.velocity = 0;
		if (!channelPedalDown) {
			noteStatus.isPedalOn = false;
		}
	} else if (status == MIDI_POLY_AFTERTOUCH) {
		noteStatus.velocity = event.value;
	} else if (oldChannelPedalDown && !channelPedalDown) { // Pedal released
		for (auto & keyStatus : keyStatuses[track][ch]) {
			if (!keyStatus.isOn) {
				keyStatus.isPedalOn = false;
			}
		}
	}

	// note history
	auto & noteHistVector = noteHistories[track][ch];
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
			if (!channelPedalDown) {
				noteHistory->pedalOffTime = currentTime;
			}
		}
	} else if (oldChannelPedalDown && !channelPedalDown) { // Pedal released
		for (auto & noteHistory : noteHistVector) {
			if (!noteHistory.pedalOffTime && noteHistory.offTime) {
				noteHistory.pedalOffTime = currentTime;
			}
		}
	}
}
