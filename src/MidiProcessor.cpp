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

void MidiProcessor::setPedalRouting(const std::map<std::pair<int, int>, std::pair<int, int>> & routing) {
	pedalRouting = routing;
	pedalSubscribers.clear();
	for (const auto & [sub, src] : routing)
		pedalSubscribers[src].push_back(sub);
}

void MidiProcessor::processMidiMessage(ofxMidiMessage & event, uint8_t track, int64_t timestamp) {
	if (track >= channels.size()) {
		ofLogWarning() << "processMidiMessage: track " << (int)track + 1 << " out of range";
		return;
	}

	ofLogNotice() << "Track " << (int)track + 1 << ": " << event.toString();

	MidiStatus status = event.status;
	ChannelState & channelState = channels[track][event.channel - 1];
	bool & channelPedalDown = channelState.pedalDown;
	auto & noteHistories = channelState.noteHistories;

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
	bool oldChannelPedalDown = channelPedalDown;
	if (status == MIDI_CONTROL_CHANGE && event.control == 64)
		channelPedalDown = (event.value >= 64);

	// Effective pedal: use redirected source if routing is configured for this channel
	auto routeIt = pedalRouting.find({ (int)track, event.channel - 1 });
	bool useRedirectedPedal = (routeIt != pedalRouting.end());
	bool effectivePedalDown = useRedirectedPedal
		? channels[routeIt->second.first][routeIt->second.second].pedalDown
		: channelPedalDown;

	// Pedal released
	if (oldChannelPedalDown && !channelPedalDown) {
		auto releasePedalForState = [&](ChannelState & state) {
			for (auto & keyStatus : state.keyStatuses) {
				if (!keyStatus.isOn) keyStatus.isPedalOn = false;
			}
			for (auto & nh : state.noteHistories) {
				if (nh.pedalOffTime == MAX_TIME && nh.offTime < MAX_TIME)
					nh.pedalOffTime = timestamp;
			}
		};

		if (!useRedirectedPedal) releasePedalForState(channelState);

		// Fan out pedal release to subscriber channels
		auto subsIt = pedalSubscribers.find({ (int)track, event.channel - 1 });
		if (subsIt != pedalSubscribers.end()) {
			for (auto & [t2, c2] : subsIt->second) {
				auto & subState = channels[t2][c2];
				releasePedalForState(subState);
			}
		}
	}

	// note status
	if (status == MIDI_NOTE_ON && event.velocity > 0) {
		auto & noteStatus = channelState.keyStatuses[event.pitch];
		noteStatus.isOn = true;
		noteStatus.velocity = event.velocity;
	} else if (status == MIDI_NOTE_OFF || (status == MIDI_NOTE_ON && event.velocity == 0)) {
		auto & noteStatus = channelState.keyStatuses[event.pitch];
		noteStatus.isOn = false;
		noteStatus.velocity = 0;
		if (!effectivePedalDown) {
			noteStatus.isPedalOn = false;
		}
	} else if (status == MIDI_POLY_AFTERTOUCH) {
		auto & noteStatus = channelState.keyStatuses[event.pitch];
		noteStatus.velocity = event.value;
	}

	// note history
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
			if (!effectivePedalDown) noteHistory->pedalOffTime = timestamp;
		}
	}
}
