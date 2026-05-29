#include "ConfigLoader.h"

#include "ofFileUtils.h"
#include "ofLog.h"
#include "yaml-cpp/yaml.h"

#include <cctype>
#include <filesystem>
#include <regex>
#include <sstream>

// ── Scalar parsers ────────────────────────────────────────────────────────────

static ofColor parseHexColor(const std::string & s) {
	// Accepts "#rrggbb" (with or without leading #)
	std::string hex = s;
	if (!hex.empty() && hex[0] == '#') hex = hex.substr(1);
	if (hex.size() != 6) {
		ofLogWarning("ConfigLoader") << "Invalid hex color: " << s;
		return ofColor(255, 255, 255);
	}
	int r = std::stoi(hex.substr(0, 2), nullptr, 16);
	int g = std::stoi(hex.substr(2, 2), nullptr, 16);
	int b = std::stoi(hex.substr(4, 2), nullptr, 16);
	return ofColor(r, g, b);
}

static NoteRenderMode parseNoteMode(const std::string & s, int & ccNumberOut) {
	std::string lower = s;
	std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
	if (lower == "fill") return NoteRenderMode::Fill;
	if (lower == "decay") return NoteRenderMode::Decay;
	// CC# e.g. "CC9", "cc11"
	std::smatch m;
	if (std::regex_match(lower, m, std::regex("cc(\\d+)"))) {
		ccNumberOut = std::stoi(m[1]);
		return NoteRenderMode::CC;
	}
	ofLogWarning("ConfigLoader") << "Unknown note mode: " << s << " — defaulting to decay";
	return NoteRenderMode::Decay;
}

static PlaybackMode parsePlaybackMode(const std::string & s) {
	if (s == "live") return PlaybackMode::Live;
	if (s == "export") return PlaybackMode::Export;
	return PlaybackMode::Preview;
}

// Parses "1,3-5" (1-based) → {0, 2, 3, 4} (0-based)
static std::vector<int> parseRanges(const std::string & s) {
	std::vector<int> result;
	std::istringstream ss(s);
	std::string token;
	while (std::getline(ss, token, ',')) {
		// trim
		token.erase(0, token.find_first_not_of(" \t"));
		token.erase(token.find_last_not_of(" \t") + 1);
		size_t dash = token.find('-');
		if (dash != std::string::npos) {
			int start = std::stoi(token.substr(0, dash));
			int end = std::stoi(token.substr(dash + 1));
			for (int i = start; i <= end; ++i)
				result.push_back(i);
		} else {
			result.push_back(std::stoi(token));
		}
	}
	return result;
}

// ── Style parsers ─────────────────────────────────────────────────────────────

static RemapEntry parseRemapEntry(const YAML::Node & node) {
	RemapEntry e;
	if (node["pitch"]) e.pitch = node["pitch"].as<int>();
	if (node["suppressed"]) e.suppressed = node["suppressed"].as<bool>();
	if (node["mapStartPitch"]) e.mapStartPitch = node["mapStartPitch"].as<int>();
	if (node["mapEndPitch"]) e.mapEndPitch = node["mapEndPitch"].as<int>();
	if (node["color"]) {
		e.color = parseHexColor(node["color"].as<std::string>());
		e.hasColor = true;
	}
	return e;
}

static Style parseStyle(const YAML::Node & node) {
	Style s;
	if (!node || !node.IsMap()) return s;

	if (node["note"]) {
		const auto & n = node["note"];
		if (n["mode"]) s.note.mode = parseNoteMode(n["mode"].as<std::string>(), s.note.ccNumber);
		if (n["velocity"]) s.note.velocity = n["velocity"].as<bool>();
		if (n["color"]) s.note.color = parseHexColor(n["color"].as<std::string>());
		if (n["decay"]) {
			const auto & d = n["decay"];
			if (d["rate"]) s.note.decay.rate = d["rate"].as<float>();
			if (d["timeSegment"]) s.note.decay.timeSegment = d["timeSegment"].as<int64_t>();
		}
		if (n["gap"]) s.note.gap = n["gap"].as<float>();
		if (n["radius"]) s.note.radius = n["radius"].as<float>();
		if (n["z"]) s.note.z = n["z"].as<float>();
	}

	if (node["pedal"]) {
		const auto & p = node["pedal"];
		if (p["show"]) s.pedal.show = p["show"].as<bool>();
		if (p["color"]) s.pedal.color = parseHexColor(p["color"].as<std::string>());
		if (p["track"]) s.pedal.track = p["track"].as<int>();
		if (p["channel"]) s.pedal.channel = p["channel"].as<int>();
	}

	if (node["remaps"] && node["remaps"].IsSequence()) {
		for (const auto & item : node["remaps"])
			s.remaps.push_back(parseRemapEntry(item));
	}

	return s;
}

// Like parseStyle but only sets fields that are explicitly present (for track/channel overrides)
static StyleOverride parseStyleOverride(const YAML::Node & node) {
	StyleOverride s;
	if (!node || !node.IsMap()) return s;

	if (node["note"] && node["note"].IsMap()) {
		NoteStyleOverride n;
		const auto & nn = node["note"];
		if (nn["mode"]) {
			int ccNum = 0;
			n.mode = parseNoteMode(nn["mode"].as<std::string>(), ccNum);
			n.ccNumber = ccNum;
		}
		if (nn["velocity"]) n.velocity = nn["velocity"].as<bool>();
		if (nn["color"]) n.color = parseHexColor(nn["color"].as<std::string>());
		if (nn["decay"]) {
			const auto & d = nn["decay"];
			if (d["rate"]) n.decay.rate = d["rate"].as<float>();
			if (d["timeSegment"]) n.decay.timeSegment = d["timeSegment"].as<int64_t>();
		}
		if (nn["gap"]) n.gap = nn["gap"].as<float>();
		if (nn["radius"]) n.radius = nn["radius"].as<float>();
		if (nn["z"]) n.z = nn["z"].as<float>();
		s.note = n;
	}

	if (node["pedal"] && node["pedal"].IsMap()) {
		PedalStyleOverride p;
		const auto & pp = node["pedal"];
		if (pp["show"]) p.show = pp["show"].as<bool>();
		if (pp["color"]) p.color = parseHexColor(pp["color"].as<std::string>());
		if (pp["track"]) p.track = pp["track"].as<int>();
		if (pp["channel"]) p.channel = pp["channel"].as<int>();
		s.pedal = p;
	}

	if (node["remaps"] && node["remaps"].IsSequence()) {
		for (const auto & item : node["remaps"])
			s.remaps.push_back(parseRemapEntry(item));
	}

	return s;
}

// ── Loader ────────────────────────────────────────────────────────────────────

VisualizerConfig ConfigLoader::load(const std::string & path) {
	VisualizerConfig cfg;

	if (!ofFile::doesFileExist(path)) {
		ofLogWarning("ConfigLoader") << "Config file not found: " << path << " — using defaults.";
		return cfg;
	}

	auto configDir = std::filesystem::absolute(path).parent_path();
	auto resolvePath = [&](const std::string & p) -> std::string {
		if (p.empty() || std::filesystem::path(p).is_absolute()) return p;
		return (configDir / p).string();
	};

	try {
		YAML::Node root = YAML::LoadFile(path);

		// playback
		if (root["playback"]) {
			const auto & pb = root["playback"];
			if (pb["mode"]) cfg.playback.mode = parsePlaybackMode(pb["mode"].as<std::string>());
			if (pb["midiFilePath"]) cfg.playback.midiFilePath = resolvePath(pb["midiFilePath"].as<std::string>());
			if (pb["audioFilePath"]) cfg.playback.audioFilePath = resolvePath(pb["audioFilePath"].as<std::string>());
			if (pb["midiInPort"]) cfg.playback.midiInPort = pb["midiInPort"].as<int>();
			if (pb["startPadding"]) cfg.playback.startPadding = pb["startPadding"].as<int64_t>();
			if (pb["endPadding"]) cfg.playback.endPadding = pb["endPadding"].as<int64_t>();
		}

		// export
		if (root["export"]) {
			const auto & ex = root["export"];
			if (ex["path"]) cfg.exportCfg.path = ex["path"].as<std::string>();
			if (ex["fps"]) cfg.exportCfg.fps = ex["fps"].as<int>();
			if (ex["limit"]) cfg.exportCfg.limit = ex["limit"].as<int64_t>();
		}

		// timing
		if (root["timing"]) {
			const auto & t = root["timing"];
			if (t["dispatchOffset"]) cfg.timing.dispatchOffset = t["dispatchOffset"].as<int64_t>();
			if (t["removeOffset"]) cfg.timing.removeOffset = t["removeOffset"].as<int64_t>();
			if (t["audioOffset"]) cfg.timing.audioOffset = t["audioOffset"].as<int64_t>();
		}

		// camera
		if (root["camera"]) {
			const auto & c = root["camera"];
			if (c["pitchAxis"]) cfg.camera.pitchAxis = c["pitchAxis"].as<float>();
			if (c["timeAxis"]) cfg.camera.timeAxis = c["timeAxis"].as<float>();
			if (c["zAxis"]) cfg.camera.zAxis = c["zAxis"].as<float>();
			if (c["targetPitchAxis"]) cfg.camera.targetPitchAxis = c["targetPitchAxis"].as<float>();
			if (c["targetTimeAxis"]) cfg.camera.targetTimeAxis = c["targetTimeAxis"].as<float>();
		}

		// display
		if (root["display"]) {
			const auto & d = root["display"];
			if (d["width"]) cfg.display.width = d["width"].as<int>();
			if (d["height"]) cfg.display.height = d["height"].as<int>();
			if (d["showPiano"]) cfg.display.showPiano = d["showPiano"].as<bool>();
			if (d["averageWidth"]) cfg.display.averageWidth = d["averageWidth"].as<bool>();
			if (d["reverse"]) cfg.display.reverse = d["reverse"].as<bool>();
			if (d["horizontal"]) cfg.display.horizontal = d["horizontal"].as<bool>();
			if (d["speed"]) cfg.display.speed = d["speed"].as<float>();
			if (d["beat"]) cfg.display.beat = d["beat"].as<bool>();
		}

		if (root["debug"]) cfg.debug = root["debug"].as<bool>();

		// global style
		if (root["style"]) cfg.style = parseStyle(root["style"]);

		// per-track overrides
		if (root["tracks"] && root["tracks"].IsSequence()) {
			for (const auto & tNode : root["tracks"]) {
				TrackConfig tc;
				if (tNode["track"]) tc.tracks = parseRanges(tNode["track"].as<std::string>());
				if (tNode["regex"]) tc.regex = tNode["regex"].as<std::string>();
				if (tNode["style"]) tc.style = parseStyleOverride(tNode["style"]);

				if (tNode["channels"] && tNode["channels"].IsSequence()) {
					for (const auto & cNode : tNode["channels"]) {
						ChannelConfig cc;
						if (cNode["channel"]) cc.channels = parseRanges(cNode["channel"].as<std::string>());
						if (cNode["style"]) cc.style = parseStyleOverride(cNode["style"]);
						tc.channels.push_back(std::move(cc));
					}
				}
				cfg.tracks.push_back(std::move(tc));
			}
		}

	} catch (const YAML::Exception & e) {
		ofLogError("ConfigLoader") << "Failed to parse " << path << ": " << e.what() << " — using defaults.";
		return VisualizerConfig {};
	}

	return cfg;
}
