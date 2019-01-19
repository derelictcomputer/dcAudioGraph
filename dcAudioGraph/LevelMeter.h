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

	void setNumAudioIo(size_t num, bool isInput) override;
	void setNumControlIo(size_t num, bool isInput) override {}

protected:
	void process() override;

	std::vector<float> _levels;
};
}
