#pragma once

#include "VisualizerConfig.h"
#include <string>

struct ConfigLoader {
	// Loads VisualizerConfig from a YAML file.
	// If the file does not exist, returns a default-constructed VisualizerConfig.
	static VisualizerConfig load(const std::string & path);
};
