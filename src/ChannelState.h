#pragma once

#include <array>
#include <deque>

#include "midiUtil.h"

struct ChannelState {
	std::array<keyStatus_t, 128> keyStatuses{};
	std::deque<noteHistory_t> noteHistories;
	std::deque<channelHistory_t> channelHistories;
	bool pedalDown = false;
};
