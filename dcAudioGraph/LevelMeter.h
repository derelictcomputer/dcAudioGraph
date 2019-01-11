#pragma once

#include "Module.h"

namespace dc
{
class LevelMeter : public Module
{
public:
	LevelMeter() = default;

	std::string getName() override { return "Level Meter"; }

	float getLevel(size_t channel);

protected:
	void onProcess() override;
	void onRefreshAudioBuffers() override;

	std::vector<float> _levels;
};
}
