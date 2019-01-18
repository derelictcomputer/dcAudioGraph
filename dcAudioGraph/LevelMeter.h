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
	LevelMeter();

	float getLevel(size_t channel);

protected:
	void process() override;

	std::vector<float> _levels;
};
}
