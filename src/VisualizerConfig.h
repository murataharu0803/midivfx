#pragma once

#include "ofColor.h"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// ── Enums ─────────────────────────────────────────────────────────────────────

enum class PlaybackMode {
	Preview,
	Live,
	Export
};
enum class NoteRenderMode {
	Fill,
	Decay,
	CC
};

// ── Style structs (reusable, nestable) ────────────────────────────────────────

struct DecayStyle {
	float rate = 0.95f;
	int64_t timeSegment = 100'000;
};

struct NoteStyle {
	NoteRenderMode mode = NoteRenderMode::Decay;
	bool velocity = true;
	ofColor color = ofColor(255, 160, 0);
	int ccNumber = 0; // CC number when mode == CC
	DecayStyle decay;
};

struct PedalStyle {
	bool show = true;
	ofColor color = ofColor(160, 160, 160);
	int track = 0; // 1-based; 0 = use original track
	int channel = 0; // 1-based; 0 = use original channel
};

struct RemapEntry {
	uint8_t pitch = 0;
	bool suppressed = false;
	uint8_t mapStartPitch = 0;
	uint8_t mapEndPitch = 0;
	ofColor color;
	bool hasColor = false;
};

struct Style {
	NoteStyle note;
	PedalStyle pedal;
	std::vector<RemapEntry> remaps;
};

// ── Per-track / per-channel overrides ────────────────────────────────────────

struct ChannelConfig {
	std::vector<int> channels; // 1-based, resolved from range string
	Style style;
};

struct TrackConfig {
	std::vector<int> tracks; // 1-based; empty = apply to all (or match by regex)
	std::string regex;
	Style style;
	std::vector<ChannelConfig> channels;
};

// ── Top-level config sections ─────────────────────────────────────────────────

struct PlaybackConfig {
	PlaybackMode mode = PlaybackMode::Preview;
	std::string midiFilePath = "song.mid";
	int midiInPort = 1;
	int64_t startPadding = 3'000'000;
	int64_t endPadding = 3'000'000;
};

struct ExportConfig {
	std::string path = "output.mp4";
	int fps = 60;
	int64_t limit = 0; // 0 = no limit
};

struct TimingConfig {
	int64_t dispatchOffset = 5'000'000;
	int64_t removeOffset = 5'000'000;
};

struct CameraConfig {
	float pitchAxis = 0.f;
	float timeAxis = 500.f;
	float zAxis = 3200.f;
	float targetPitchAxis = 0.f;
	float targetTimeAxis = 500.f;
};

struct DisplayConfig {
	int width = 1920;
	int height = 1080;
	bool showPiano = false;
	bool averageWidth = true;
	bool reverse = true;
	bool horizontal = true;
	float speed = 0.001f;
};

// ── Root config ───────────────────────────────────────────────────────────────

struct VisualizerConfig {
	PlaybackConfig playback;
	ExportConfig exportCfg; // named exportCfg to avoid C++ 'export' keyword conflict
	TimingConfig timing;
	CameraConfig camera;
	DisplayConfig display;
	Style style;
	std::vector<TrackConfig> tracks;
};
