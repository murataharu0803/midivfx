#include <filesystem>

#include "ConfigLoader.h"
#include "ofApp.h"
#include "ofMain.h"

int main(int argc, char * argv[]) {
	// Resolve config path: --config <path> overrides default (next to executable)
	std::string configPath = ofFilePath::getCurrentExeDir() + "config.yaml";
	for (int i = 1; i < argc; ++i) {
		if (std::string(argv[i]) == "--config" && i + 1 < argc) {
			configPath = std::filesystem::absolute(argv[++i]).string();
		}
	}

	VisualizerConfig cfg = ConfigLoader::load(configPath);

	ofGLWindowSettings settings;
	settings.setSize(cfg.display.width, cfg.display.height);
	settings.windowMode = OF_WINDOW;

	auto window = ofCreateWindow(settings);
	auto app = std::make_shared<ofApp>();
	app->config = std::move(cfg);

	ofRunApp(window, app);
	ofRunMainLoop();
}
