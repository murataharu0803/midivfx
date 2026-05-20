#pragma once

#include <string>
#include <vector>

#include "midiUtil.h"

struct MidiLoadResult {
	std::vector<MidiFileEvent> events;
	std::vector<BeatEvent> beatEvents;
	int trackCount = 0;
};

class MidiFileLoader {
public:
	static MidiLoadResult load(const std::string & path);
};
