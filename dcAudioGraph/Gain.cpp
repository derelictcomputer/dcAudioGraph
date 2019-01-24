#include "Gain.h"

void dc::Gain::GainProcessor::process(AudioBuffer& audioBuffer, ControlBuffer& controlBuffer)
{
	const float currentGain = getParamValue(0);

    if (_updateGainParam)
    {
        // TODO: do we need a configurable ramp time for the gain knob?
		const float gainPerSample = (currentGain - _lastGainValue) / audioBuffer.getNumSamples();
		float gain = _lastGainValue;
        for (size_t cIdx = 0; cIdx < audioBuffer.getNumChannels(); ++cIdx)
        {
			auto* cPtr = audioBuffer.getChannelPointer(cIdx);
            for (size_t sIdx = 0; sIdx < audioBuffer.getNumSamples(); ++sIdx)
            {
				cPtr[sIdx] *= gain;
				gain += gainPerSample;
            }
        }
		_updateGainParam = false;
    }
    // if nothing changed, just apply the gain directly
    else
    {
		audioBuffer.applyGain(currentGain);
    }

	_lastGainValue = currentGain;
}

void dc::Gain::GainProcessor::paramValueChanged(size_t)
{
	_updateGainParam = true;
}

dc::Gain::Gain() : Module(std::make_unique<GainProcessor>())
{
	addParam("gain", "Gain", ParamRange(0.0f, 2.0f, 0.0f), true, true);
	setParamValue("gain", 1.0f);
}
