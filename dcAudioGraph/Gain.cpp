#include "Gain.h"
#include <cmath>

dc::Gain::Gain()
{
	setNumIo(Audio | Input | Output, 1);
	addParam("gain", "Gain", ParamRange(-70.0f, 0.0f, 0.0f, getNormalized, getRaw, 2.0f), true, true, 1.0f);
}

void dc::Gain::process(ModuleProcessContext& context)
{
    updateParams(context);

	const size_t nChannels = context.audioBuffer.getNumChannels();
	const size_t nSamples = context.audioBuffer.getNumSamples();

    for (size_t cIdx = 0; cIdx < nChannels; ++cIdx)
    {
		auto* audPtr = context.audioBuffer.getChannelPointer(cIdx);
        for (size_t sIdx = 0; sIdx < nSamples; ++sIdx)
        {
			const float gainDb = context.params[0]->getSmoothedRaw(sIdx);
			audPtr[sIdx] *= dbToLin(gainDb);
        }
    }
}

float dc::Gain::getNormalized(float rawValue, float min, float max)
{
    const float pct = (rawValue - min) / (max - min);
	return powf(pct, 2.0f);
}

float dc::Gain::getRaw(float normalizedValue, float min, float max)
{
	normalizedValue = expf(std::log(normalizedValue) / 2.0f);
	return min + (max - min) * normalizedValue;
}

float dc::Gain::dbToLin(float db)
{
	return powf(10.0f, db / 20.0f);
}
