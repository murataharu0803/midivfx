#include "ofApp.h"
#include "ofMain.h"
#include "VisualizerConfig.h"
#include <string>

int main(int argc, char* argv[]) {
	ofGLWindowSettings settings;
	settings.setSize(1920, 1080);
	settings.windowMode = OF_WINDOW;

	auto window = ofCreateWindow(settings);
	auto app = std::make_shared<ofApp>();

	for (int i = 1; i < argc; ++i) {
		std::string arg = argv[i];
		if (arg == "--export") {
			app->config.playback.mode = PlaybackMode::Export;
		} else if (arg == "--output" && i + 1 < argc) {
			app->config.exportCfg.path = argv[++i];
		} else if (arg == "--fps" && i + 1 < argc) {
			app->config.exportCfg.fps = std::stoi(argv[++i]);
		}
	}

	ofRunApp(window, app);
	ofRunMainLoop();
}
