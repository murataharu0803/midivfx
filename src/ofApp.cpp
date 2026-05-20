#include <algorithm>

#include "ofApp.h"

const int MAX_HISTORY_SIZE = 1024;
const int64_t MAX_TIME = std::numeric_limits<int64_t>::max();
const float speed = .001f;

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
			smf.linkNotePairs(); // populates ev.getLinkedEvent() for note-ons

			// Extract beat timestamps with bar position
			int tpq = smf.getTPQ();
			int maxTick = 0;
			for (int t = 0; t < smf.getTrackCount(); ++t) {
				if (smf[t].size() > 0) maxTick = std::max(maxTick, smf[t].last().tick);
			}

			// Collect time signature events (meta type 0x58)
			struct TimeSig {
				int tick;
				int num;
				int denPow;
			};
			std::vector<TimeSig> timeSigs;
			for (int t = 0; t < smf.getTrackCount(); ++t) {
				for (int i = 0; i < smf[t].size(); ++i) {
					smf::MidiEvent & ev = smf[t][i];
					if (ev.isMetaMessage() && ev[1] == 0x58 && ev.size() >= 6) {
						timeSigs.push_back({ ev.tick, (int)(uint8_t)ev[3], (int)(uint8_t)ev[4] });
					}
				}
			}
			std::sort(timeSigs.begin(), timeSigs.end(), [](const TimeSig & a, const TimeSig & b) {
				return a.tick < b.tick;
			});

			// Remove duplicates at the same tick (keep last)
			timeSigs.erase(std::unique(timeSigs.begin(), timeSigs.end(), [](const TimeSig & a, const TimeSig & b) {
				return a.tick == b.tick;
			}),
				timeSigs.end());
			if (timeSigs.empty() || timeSigs[0].tick > 0) {
				timeSigs.insert(timeSigs.begin(), { 0, 4, 2 }); // default 4/4 from start
			}

			// Generate beat events segment by segment (one segment per time signature)
			int sigCount = (int)timeSigs.size();
			for (int si = 0; si < sigCount; ++si) {
				int segStart = timeSigs[si].tick;
				int segEnd = (si + 1 < sigCount) ? timeSigs[si + 1].tick : maxTick + 1;
				// Number of quarter-note beats per bar for this time signature
				int den = 1 << timeSigs[si].denPow;
				int quarterBeatsPerBeat = 4 / den;
				int quarterBeatsPerBar = timeSigs[si].num * quarterBeatsPerBeat;
				if (quarterBeatsPerBar <= 0) quarterBeatsPerBar = 4;

				for (int tick = segStart; tick < segEnd && tick <= maxTick; tick += tpq * quarterBeatsPerBeat) {
					int beatInBar = ((tick - segStart) / tpq) % quarterBeatsPerBar;
					beatEvents.push_back({ (int64_t)(smf.getTimeInSeconds(tick) * 1'000'000), (float)beatInBar });
				}
			}

			initTracks(smf.getTrackCount());

			for (int t = 0; t < smf.getTrackCount(); ++t) {
				for (int i = 0; i < smf[t].size(); ++i) {
					smf::MidiEvent & ev = smf[t][i];
					if (!ev.isNoteOn() && !ev.isNoteOff() && !ev.isController()) continue;

					int64_t onTimeUs = (int64_t)(ev.seconds * 1'000'000);
					int64_t offTimeUs = onTimeUs + 10'000;
					if (ev.isNoteOn() && ev.getVelocity() > 0 && ev.isLinked()) {
						offTimeUs = (int64_t)(ev.getLinkedEvent()->seconds * 1'000'000);
					}

					midiFileEvents.push_back({
						onTimeUs,
						offTimeUs,
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
	currentTime = ofGetElapsedTimeMicros() + playbackStartTime;

	if (useMidiFile) {
		int64_t lookaheadTime = currentTime + dispatchOffset;
		while (playbackHead < midiFileEvents.size() && midiFileEvents[playbackHead].timeUs <= lookaheadTime) {
			auto & e = midiFileEvents[playbackHead++];
			ofxMidiMessage msg;
			msg.status = (MidiStatus)(e.status);
			msg.channel = e.channel + 1; // processMidiMessage expects 1-based
			msg.pitch = e.data1;
			msg.velocity = e.data2;
			msg.value = e.data2;
			processMidiMessage(msg, e.track, e.timeUs);
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

	// Remove old note history entries (past removeOffset after pedalOffTime)
	for (auto & trackHistories : noteHistories) {
		for (auto & noteHistVector : trackHistories) {
			while (!noteHistVector.empty()) {
				int64_t pedalOffTime = noteHistVector.front().pedalOffTime;
				if (pedalOffTime < currentTime - removeOffset) {
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

	// Draw beat lines
	if (!beatEvents.empty()) {
		const float totalWidth = PianoKey::getKeysWidth(0, 127);
		const int64_t visibleStart = currentTime - removeOffset;
		const int64_t visibleEnd = currentTime + dispatchOffset;

		auto toY = [&](int64_t t) -> float {
			return reverseMode
				? (float)(t - currentTime) * speed
				: (float)(currentTime - t) * speed;
		};

		ofPushStyle();
		ofDisableLighting();
		for (const auto & beatEvent : beatEvents) {
			if (beatEvent.timeUs < visibleStart || beatEvent.timeUs > visibleEnd) continue;
			float y = toY(beatEvent.timeUs);
			bool isDownbeat = (beatEvent.beatInBar == 0.0f);
			ofSetColor(isDownbeat ? ofColor(255, 255, 255, 140) : ofColor(255, 255, 255, 40));
			ofSetLineWidth(isDownbeat ? 2.0f : 1.0f);
			ofDrawLine(-totalWidth / 2, y, 0, totalWidth / 2, y, 0);
		}
		ofEnableLighting();
		ofPopStyle();
	}

	// Draw piano keys on top
	pianoKeys.draw(currentTime, reverseMode, dispatchOffset, removeOffset);

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
	processMidiMessage(event, 0, currentTime); // live MIDI always maps to track 0
}

void ofApp::processMidiMessage(ofxMidiMessage & event, uint8_t track, int64_t timestamp) {
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
		timestamp,
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
				return nh.pitch == event.pitch && nh.offTime >= MAX_TIME;
			});
		if (noteHistory == noteHistVector.end()) {
			// find one that is still pedaled
			auto pedalNoteHistory = std::find_if(
				noteHistVector.begin(),
				noteHistVector.end(),
				[&](const noteHistory_t & nh) {
					return nh.pitch == event.pitch && nh.offTime && nh.pedalOffTime >= MAX_TIME;
				});
			if (pedalNoteHistory != noteHistVector.end()) {
				pedalNoteHistory->pedalOffTime = timestamp;
			}
			// first check size limit
			if (noteHistVector.size() > MAX_HISTORY_SIZE - 1) {
				noteHistVector.pop_front();
			}
			// and then create history
			noteHistVector.push_back({
				timestamp,
				MAX_TIME,
				MAX_TIME,
				static_cast<uint8_t>(event.pitch),
				static_cast<uint8_t>(event.velocity),
			});
		}
	} else if (status == MIDI_NOTE_OFF || (status == MIDI_NOTE_ON && event.velocity == 0)) {
		auto noteHistory = std::find_if(
			noteHistVector.begin(),
			noteHistVector.end(),
			[&](const noteHistory_t & nh) {
				return nh.pitch == event.pitch && nh.offTime >= MAX_TIME;
			});
		if (noteHistory != noteHistVector.end()) {
			noteHistory->offTime = timestamp;
			if (!channelPedalDown) {
				noteHistory->pedalOffTime = timestamp;
			}
		}
	} else if (oldChannelPedalDown && !channelPedalDown) { // Pedal released
		for (auto & noteHistory : noteHistVector) {
			if (!noteHistory.pedalOffTime && noteHistory.offTime) {
				noteHistory.pedalOffTime = timestamp;
			}
		}
	}
}
