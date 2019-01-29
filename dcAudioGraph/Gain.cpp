#include "Gain.h"

dc::Gain::Gain()
{
	setNumIo(Audio | Input | Output, 1);
	addParam("gain", "Gain", ParamRange(0.0f, 1.0f, 0.0f), true, true);
	setParamValue("gain", 1.0f);
	_lastKnobValue = 1.0f;
}

void dc::Gain::process(ModuleProcessContext& context)
{
	const size_t nChannels = context.audioBuffer.getNumChannels();
	const size_t nSamples = context.audioBuffer.getNumSamples();

	const float knobStartValue = _lastKnobValue;
	const float knobEndValue = context.params[0]->getNormalized();
	_lastKnobValue = knobEndValue;
	const float knobInc = (knobEndValue - knobStartValue) / nSamples;

	auto* ctlPtr = context.controlBuffer.getChannelPointer(0);
	ParamRange gainRange = context.params[0]->getRange();

    for (size_t cIdx = 0; cIdx < nChannels; ++cIdx)
    {
		auto* audPtr = context.audioBuffer.getChannelPointer(cIdx);
        for (size_t sIdx = 0; sIdx < nSamples; ++sIdx)
        {
			const float knobValue = knobStartValue + sIdx * knobInc;
			const float combinedValue = knobValue + ctlPtr[sIdx] * (1.0f - knobValue);
			const float gain = gainRange.getRaw(combinedValue);
			audPtr[sIdx] *= gain;
        }
    }
}
