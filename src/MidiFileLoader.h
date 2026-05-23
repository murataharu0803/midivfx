#pragma once

#include <string>
#include <vector>

#include "midiUtil.h"

struct MidiLoadResult {
	std::vector<MidiFileEvent> events;
	std::vector<BeatEvent> beatEvents;
	std::vector<std::string> trackNames; // one per track (empty string if no name meta)
	int trackCount = 0;
};

class MidiFileLoader {
public:
	static MidiLoadResult load(const std::string & path);
};
