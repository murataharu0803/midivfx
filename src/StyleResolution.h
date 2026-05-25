#include "VisualizerConfig.h"

class StyleResolution {
public:
	static Style resolveStyle(const VisualizerConfig & config,
		const std::vector<std::string> & trackNames,
		int track0, int channel0);

private:
	static Style applyOverride(Style base, const StyleOverride & over);
	static bool trackMatches(const TrackConfig & tc, int track0, const std::vector<std::string> & trackNames);
};
