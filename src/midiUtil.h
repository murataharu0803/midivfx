struct keyStatus_t {
	int velocity; // also used for polyAftertouch
	bool isOn;
	// TODO: CC values
};

struct channelHistory_t {
	uint64_t timestamp;
	MidiStatus status;
	uint8_t pitch; // only set for noteOn/noteOff/polyAftertouch
	uint8_t control; // only set for CC
	uint8_t value; // velocity for noteOn/noteOff/polyAftertouch, value for CC
};
