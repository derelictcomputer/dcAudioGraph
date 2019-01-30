#include "Gain.h"

dc::Gain::Gain()
{
	setNumIo(Audio | Input | Output, 1);
	addParam("gain", "Gain", ParamRange(0.0f, 1.0f, 0.0f), true, true, 1.0f);
}

void dc::Gain::process(ModuleProcessContext& context)
{
	const size_t nChannels = context.audioBuffer.getNumChannels();
	const size_t nSamples = context.audioBuffer.getNumSamples();

	context.params[0]->updateSmoothing(nSamples);

	auto* ctlPtr = context.controlBuffer.getChannelPointer(0);

    for (size_t cIdx = 0; cIdx < nChannels; ++cIdx)
    {
		auto* audPtr = context.audioBuffer.getChannelPointer(cIdx);
        for (size_t sIdx = 0; sIdx < nSamples; ++sIdx)
        {
			audPtr[sIdx] *= context.params[0]->getSmoothedRaw(sIdx, ctlPtr[sIdx]);
        }
    }
}
