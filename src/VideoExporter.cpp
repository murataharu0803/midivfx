#include "VideoExporter.h"
#include <algorithm>
#include <cstdio>

void VideoExporter::setup(const VisualizerConfig & cfg, const std::vector<MidiFileEvent> & events) {
	if (cfg.playback.mode != PlaybackMode::Export) return;

	config = &cfg;
	active = true;

	ofSetVerticalSync(false);
	ofSetFrameRate(0);

	fbo.allocate(config->display.width, config->display.height, GL_RGB);

	endTimeUs = 0;
	for (auto & e : events) {
		endTimeUs = std::max(endTimeUs, std::max(e.timeUs, e.offTimeUs));
	}
	endTimeUs += config->playback.endPadding;
	if (config->exportCfg.limit > 0) {
		endTimeUs = std::min(endTimeUs, config->exportCfg.limit);
	}

	ofDirectory framesDir(ofToDataPath("export_frames"));
	if (!framesDir.exists()) framesDir.create();

	ofLogNotice("Export") << "Rendering " << endTimeUs / 1'000'000.0 << "s at "
						  << config->exportCfg.fps << " fps -> " << config->exportCfg.path;
}

int64_t VideoExporter::advanceAndGetElapsedUs() {
	int64_t t = syntheticElapsedUs;
	syntheticElapsedUs += 1'000'000LL / config->exportCfg.fps;
	return t;
}

void VideoExporter::begin() {
	fbo.begin();
	ofClear(0, 0, 0, 255);
}

bool VideoExporter::end() {
	fbo.end();
	fbo.draw(0, 0); // preview in window

	fbo.readToPixels(pixels);
	pixels.mirror(false, false);

	char frameName[64];
	snprintf(frameName, sizeof(frameName), "export_frames/frame_%05d.png", frameIndex);
	ofImage frame;
	frame.setFromPixels(pixels);
	frame.save(ofToDataPath(frameName));

	if (frameIndex % 60 == 0) {
		int64_t currentTime = syntheticElapsedUs; // already advanced
		ofLogNotice("Export") << "Frame " << frameIndex
							  << "  t=" << currentTime / 1'000'000.0 << "s"
							  << "  / " << endTimeUs / 1'000'000.0 << "s";
	}
	++frameIndex;

	if (syntheticElapsedUs >= endTimeUs) {
		finish();
		return true;
	}
	return false;
}

void VideoExporter::finish() {
	ofLogNotice("Export") << "Saved " << frameIndex << " frames. Running ffmpeg...";

	std::string framesPath = ofToDataPath("export_frames/frame_%05d.png");
	std::string cmd = "ffmpeg -y -framerate " + std::to_string(config->exportCfg.fps)
		+ " -i \"" + framesPath + "\"";

	bool hasAudio = !config->playback.audioFilePath.empty();
	if (hasAudio) {
		cmd += " -i \"" + config->playback.audioFilePath + "\"";
	}

	cmd += " -c:v libx264 -pix_fmt yuv420p -crf 18";
	if (hasAudio) {
		// Prepend silence so audio starts at t=0, matching the video stream.
		// adelay expects milliseconds; startPadding and audioOffset are in microseconds.
		int64_t delayMs = (config->playback.startPadding + config->timing.audioOffset) / 1000;
		cmd += " -filter_complex \"[1:a]adelay=" + std::to_string(delayMs)
			+ ":all=1[a]\" -map 0:v -map \"[a]\" -c:a aac -shortest";
	}
	cmd += " \"" + config->exportCfg.path + "\"";

	ofLogNotice("Export") << cmd;
	int ret = system(cmd.c_str());
	if (ret == 0) {
		ofLogNotice("Export") << "Done -> " << config->exportCfg.path;
	} else {
		ofLogError("Export") << "ffmpeg failed (exit " << ret << "). "
							 << "Frames are in bin/data/export_frames/";
	}

	ofDirectory(ofToDataPath("export_frames")).remove(true);

	ofExit(0);
}
