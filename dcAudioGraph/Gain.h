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

	void setNumChannels(size_t numChannels);

protected:
	void process() override;
};
}
