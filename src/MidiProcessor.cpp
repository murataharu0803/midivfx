#include <algorithm>
#include <limits>

#include "MidiProcessor.h"
#include "ofLog.h"

static const int MAX_HISTORY_SIZE = 1024;
static const int64_t MAX_TIME = std::numeric_limits<int64_t>::max();

void MidiProcessor::initTracks(int count) {
	channels.resize(count);
}

std::array<bool, 128> MidiProcessor::getActiveKeys() const {
	std::array<bool, 128> active {};
	for (const auto & trackChannels : channels) {
		for (const auto & ch : trackChannels) {
			for (int i = 0; i < 128; ++i) {
				if (ch.keyStatuses[i].isOn) active[i] = true;
			}
		}
	}
	return active;
}

void MidiProcessor::trimHistory(int64_t currentTime, int64_t removeOffset) {
	for (auto & trackChannels : channels) {
		for (auto & ch : trackChannels) {
			auto & noteHistories = ch.noteHistories;
			while (!noteHistories.empty() && noteHistories.front().pedalOffTime < currentTime - removeOffset) {
				noteHistories.pop_front();
			}
		}
	}
}

void MidiProcessor::processMidiMessage(ofxMidiMessage & event, uint8_t track, int64_t timestamp, int64_t currentTime) {
	if (track >= channels.size()) {
		ofLogWarning() << "processMidiMessage: track " << track << " out of range";
		return;
	}

	ofLogNotice() << event.toString();

	const uint8_t ch = event.channel - 1; // convert to 0-based
	MidiStatus status = event.status;
	ChannelState & channelState = channels[track][ch];

	// channel history
	auto & channelHistories = channelState.channelHistories;
	if (channelHistories.size() > MAX_HISTORY_SIZE - 1) {
		channelHistories.pop_front();
	}
	channelHistories.push_back({
		timestamp,
		status,
		static_cast<uint8_t>(event.pitch),
		static_cast<uint8_t>(event.control),
		static_cast<uint8_t>(event.value),
	});

	// pedal status
	bool & channelPedalDown = channelState.pedalDown;
	bool oldChannelPedalDown = channelPedalDown;
	if (status == MIDI_CONTROL_CHANGE && event.control == 64) {
		channelPedalDown = (event.value >= 64);
	}

	// note status
	auto & noteStatus = channelState.keyStatuses[event.pitch];
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
		for (auto & keyStatus : channelState.keyStatuses) {
			if (!keyStatus.isOn) {
				keyStatus.isPedalOn = false;
			}
		}
	}

	// note history
	auto & noteHistories = channelState.noteHistories;
	if (status == MIDI_NOTE_ON && event.velocity > 0) {
		// check if not already on
		auto noteHistory = std::find_if(
			noteHistories.begin(),
			noteHistories.end(),
			[&](const noteHistory_t & nh) {
				return nh.pitch == event.pitch && nh.offTime >= MAX_TIME;
			});
		if (noteHistory == noteHistories.end()) {
			// find one that is still pedaled
			auto pedalNoteHistory = std::find_if(
				noteHistories.begin(),
				noteHistories.end(),
				[&](const noteHistory_t & nh) {
					return nh.pitch == event.pitch && nh.offTime && nh.pedalOffTime >= MAX_TIME;
				});
			if (pedalNoteHistory != noteHistories.end()) {
				pedalNoteHistory->pedalOffTime = timestamp;
			}
			// first check size limit
			if (noteHistories.size() > MAX_HISTORY_SIZE - 1) {
				noteHistories.pop_front();
			}
			// and then create history
			noteHistories.push_back({
				timestamp,
				MAX_TIME,
				MAX_TIME,
				static_cast<uint8_t>(event.pitch),
				static_cast<uint8_t>(event.velocity),
			});
		}
	} else if (status == MIDI_NOTE_OFF || (status == MIDI_NOTE_ON && event.velocity == 0)) {
		auto noteHistory = std::find_if(
			noteHistories.begin(),
			noteHistories.end(),
			[&](const noteHistory_t & nh) {
				return nh.pitch == event.pitch && nh.offTime >= MAX_TIME;
			});
		if (noteHistory != noteHistories.end()) {
			noteHistory->offTime = timestamp;
			if (!channelPedalDown) {
				noteHistory->pedalOffTime = timestamp;
			}
		}
	} else if (oldChannelPedalDown && !channelPedalDown) { // Pedal released
		for (auto & noteHistory : noteHistories) {
			if (noteHistory.pedalOffTime == MAX_TIME && noteHistory.offTime < MAX_TIME) {
				noteHistory.pedalOffTime = timestamp;
			}
		}
	}
}
