/*
 * Provides simple RMS readings for the most recent buffer,
 * and passes through audio and control.
 */

#pragma once

#include "Module.h"

namespace dc
{
class LevelMeter : public Module
{
public:
	LevelMeter() = default;

	float getLevel(size_t channel);

protected:
	void onProcess() override;
	void onRefreshAudioBuffers() override;

	std::vector<float> _levels;
};
}
