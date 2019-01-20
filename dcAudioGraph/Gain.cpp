#include "Gain.h"

void dc::Gain::GainProcessor::process(AudioBuffer& audioBuffer, ControlBuffer& controlBuffer)
{
	audioBuffer.applyGain(getParamValue(0));
}

dc::Gain::Gain() : Module(std::make_unique<GainProcessor>())
{
	addParam("gain", "Gain", ParamRange(0.0f, 2.0f, 0.0f), true, true);
	setParamValue("gain", 1.0f);
}
