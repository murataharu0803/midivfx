#include "ofApp.h"
#include "ofMain.h"
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
			app->config.exportMode = true;
		} else if (arg == "--output" && i + 1 < argc) {
			app->config.exportOutputPath = argv[++i];
		} else if (arg == "--fps" && i + 1 < argc) {
			app->config.exportFps = std::stoi(argv[++i]);
		}
	}

	ofRunApp(window, app);
	ofRunMainLoop();
}
