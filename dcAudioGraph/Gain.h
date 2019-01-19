/*
 * A simple gain slider, which can also be used as a simple summing bus.
 */

#pragma once

#include "Module.h"

namespace dc
{
class Gain : public Module
{
public:
	Gain();

	void setNumAudioIo(size_t num, bool isInput) override;
	void setNumControlIo(size_t num, bool isInput) override {}

protected:
	void process() override;
};
}
