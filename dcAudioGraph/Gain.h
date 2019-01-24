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
	protected:
		void process(AudioBuffer& audioBuffer, ControlBuffer& controlBuffer) override;
        void paramValueChanged(size_t) override;

	private:
		bool _updateGainParam = false;
		float _lastKnobValue = 0.0f;
		float _lastControlValue = 0.0f;
	};

public:
	Gain();

};
}
