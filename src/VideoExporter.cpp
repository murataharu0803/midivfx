#include "VideoExporter.h"
#include <algorithm>
#include <cstdio>

void VideoExporter::setup(int width, int height, const VisualizerConfig & cfg, const std::vector<MidiFileEvent> & events) {
	if (!cfg.exportMode) return;

	config = &cfg;
	active = true;

	ofSetVerticalSync(false);
	ofSetFrameRate(0);

	fbo.allocate(width, height, GL_RGB);

	endTimeUs = 0;
	for (auto & e : events) {
		endTimeUs = std::max(endTimeUs, std::max(e.timeUs, e.offTimeUs));
	}
	endTimeUs += config->exportEndPaddingUs;

	ofDirectory framesDir(ofToDataPath("export_frames"));
	if (!framesDir.exists()) framesDir.create();

	ofLogNotice("Export") << "Rendering " << endTimeUs / 1'000'000.0 << "s at "
	                      << config->exportFps << " fps -> " << config->exportOutputPath;
}

int64_t VideoExporter::advanceAndGetElapsedUs() {
	int64_t t = syntheticElapsedUs;
	syntheticElapsedUs += 1'000'000LL / config->exportFps;
	return t;
}

void VideoExporter::begin() {
	fbo.begin();
}

bool VideoExporter::end() {
	fbo.end();
	fbo.draw(0, 0); // preview in window

	fbo.readToPixels(pixels);
	pixels.mirror(true, false); // correct OpenGL bottom-left origin

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
	std::string cmd = "ffmpeg -y -framerate " + std::to_string(config->exportFps)
	    + " -i \"" + framesPath + "\""
	    + " -c:v libx264 -pix_fmt yuv420p -crf 18"
	    + " \"" + config->exportOutputPath + "\"";

	ofLogNotice("Export") << cmd;
	int ret = system(cmd.c_str());
	if (ret == 0) {
		ofLogNotice("Export") << "Done -> " << config->exportOutputPath;
	} else {
		ofLogError("Export") << "ffmpeg failed (exit " << ret << "). "
		                     << "Frames are in bin/data/export_frames/";
	}

	ofDirectory(ofToDataPath("export_frames")).remove(true);

	ofExit(0);
}
