#include "Gain.h"

dc::Gain::Gain()
{
	addParam("gain", "Gain", ParamRange(0.0f, 2.0f, 0.0f), true, true);
	getParam("gain")->setRaw(1.0f);
}

void dc::Gain::onProcess()
{
	// TODO: add an "apply gain" function to the AudioBuffer class and just use that
	const auto gain = getParam("gain")->getRaw();
	for (size_t cIdx = 0; cIdx < _audioBuffer.getNumChannels(); ++cIdx)
	{
		auto* cPtr = _audioBuffer.getChannelPointer(cIdx);
		for (size_t sIdx = 0; sIdx < _audioBuffer.getNumSamples(); ++sIdx)
		{
			cPtr[sIdx] *= gain;
		}
	}
}
