#include <regex>

#include "StyleResolution.h"

Style StyleResolution::applyOverride(Style base, const StyleOverride & over) {
	if (over.note) {
		const auto & n = *over.note;
		if (n.mode) {
			base.note.mode = *n.mode;
			base.note.ccNumber = n.ccNumber.value_or(0);
		}
		if (n.velocity) base.note.velocity = *n.velocity;
		if (n.color) base.note.color = *n.color;
		if (n.decay.rate) base.note.decay.rate = *n.decay.rate;
		if (n.decay.timeSegment) base.note.decay.timeSegment = *n.decay.timeSegment;
		if (n.gap) base.note.gap = *n.gap;
		if (n.radius) base.note.radius = *n.radius;
	}
	if (over.pedal) {
		const auto & p = *over.pedal;
		if (p.show) base.pedal.show = *p.show;
		if (p.color) base.pedal.color = *p.color;
		if (p.track) base.pedal.track = *p.track;
		if (p.channel) base.pedal.channel = *p.channel;
	}
	base.remaps.insert(base.remaps.end(), over.remaps.begin(), over.remaps.end());
	return base;
}

bool StyleResolution::trackMatches(const TrackConfig & tc, int track0, const std::vector<std::string> & trackNames) {
	bool byNumber = false;
	for (int t : tc.tracks) {
		if (t - 1 == track0) {
			byNumber = true;
			break;
		}
	}
	bool byRegex = false;
	if (!tc.regex.empty() && track0 < (int)trackNames.size()) {
		try {
			std::regex re(tc.regex, std::regex_constants::icase);
			byRegex = std::regex_search(trackNames[track0], re);
		} catch (...) { }
	}
	if (tc.tracks.empty() && tc.regex.empty()) return false;
	return byNumber || byRegex;
}

Style StyleResolution::resolveStyle(const VisualizerConfig & config,
	const std::vector<std::string> & trackNames,
	int track0, int channel0) {

	Style result = config.style;

	for (const auto & tc : config.tracks) {
		if (!trackMatches(tc, track0, trackNames)) continue;
		result = applyOverride(result, tc.style);

		for (const auto & cc : tc.channels) {
			bool chMatch = cc.channels.empty();
			for (int c : cc.channels) {
				if (c - 1 == channel0) {
					chMatch = true;
					break;
				}
			}
			if (!chMatch) continue;
			result = applyOverride(result, cc.style);
		}
	}

	return result;
}
