#include <algorithm>

#include "MidiFileLoader.h"
#include "ofLog.h"
#include "ofxMidifile.h"

MidiLoadResult MidiFileLoader::load(const std::string & path) {
	MidiLoadResult result;

	ofxMidifile mf;
	if (!mf.load(path)) {
		ofLogError() << "MidiFileLoader: failed to load " << path;
		return result;
	}

	smf::MidiFile & smf = mf.get();
	int trackCount = smf.getTrackCount();
	ofLogNotice() << "Tracks: " << trackCount;
	smf.doTimeAnalysis();
	smf.linkNotePairs(); // populates ev.getLinkedEvent() for note-ons

	int tpq = smf.getTPQ();
	int maxTick = 0;
	for (int t = 0; t < trackCount; ++t) {
		if (smf[t].size() > 0) maxTick = std::max(maxTick, smf[t].last().tick);
	}

	// Collect time signature events (meta type 0x58)
	struct TimeSig {
		int tick;
		int num;
		int denPow;
	};
	std::vector<TimeSig> timeSigs;
	for (int t = 0; t < trackCount; ++t) {
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
		int den = 1 << timeSigs[si].denPow;
		int quarterBeatsPerBeat = 4 / den;
		int quarterBeatsPerBar = timeSigs[si].num * quarterBeatsPerBeat;
		if (quarterBeatsPerBar <= 0) quarterBeatsPerBar = 4;

		for (int tick = segStart; tick < segEnd && tick <= maxTick; tick += tpq * quarterBeatsPerBeat) {
			int beatInBar = ((tick - segStart) / tpq) % quarterBeatsPerBar;
			result.beatEvents.push_back({ (int64_t)(smf.getTimeInSeconds(tick) * 1'000'000), (float)beatInBar });
		}
	}

	// Extract MIDI events
	for (int t = 0; t < trackCount; ++t) {
		for (int i = 0; i < smf[t].size(); ++i) {
			smf::MidiEvent & ev = smf[t][i];
			if (!ev.isNoteOn() && !ev.isNoteOff() && !ev.isController()) continue;

			int64_t onTimeUs = (int64_t)(ev.seconds * 1'000'000);
			int64_t offTimeUs = onTimeUs + 10'000;
			if (ev.isNoteOn() && ev.getVelocity() > 0 && ev.isLinked()) {
				offTimeUs = (int64_t)(ev.getLinkedEvent()->seconds * 1'000'000);
			}

			result.events.push_back({
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
	std::sort(result.events.begin(), result.events.end(),
		[](const MidiFileEvent & a, const MidiFileEvent & b) { return a.timeUs < b.timeUs; });

	// Extract track names from meta type 0x03 (Sequence/Track Name)
	result.trackNames.resize(trackCount);
	for (int t = 0; t < trackCount; ++t) {
		for (int i = 0; i < smf[t].size(); ++i) {
			smf::MidiEvent & ev = smf[t][i];
			if (ev.isMetaMessage() && ev[1] == 0x03 && ev.size() > 2) {
				result.trackNames[t] = std::string(ev.begin() + 2, ev.end());
				break; // take first occurrence
			}
		}
	}

	result.trackCount = trackCount;
	return result;
}
