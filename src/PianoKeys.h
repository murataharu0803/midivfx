#pragma once

#include "PianoKey.h"
#include "ofMain.h"

class PianoKeys {
public:
	void setup();
	void draw();

	std::vector<PianoKey> keys;

private:
};
