/*
 * A simple gain slider, which can also be used as a simple summing bus.
 */

#pragma once

#include "Module.h"

namespace dc
{
class Gain : public Module
{
	class GainProcessor : public ModuleProcessor
	{
		void process(AudioBuffer& audioBuffer, ControlBuffer& controlBuffer) override;
	};

public:
	Gain();

};
}
