#include "Gain.h"

dc::Gain::Gain()
{
	/*
	addParam("gain", "Gain", ParamRange(0.0f, 2.0f, 0.0f), true, true);
	getParam("gain")->setRaw(1.0f);
	*/
}

void dc::Gain::setNumChannels(size_t numChannels)
{
	while (numChannels < getNumAudioInputs())
	{
		const size_t idx = getNumAudioInputs() - 1;
		removeAudioIo(idx, true);
		removeAudioIo(idx, false);
	}
	while (numChannels > getNumAudioInputs())
	{
		addAudioIo(true);
		addAudioIo(false);
	}
}

void dc::Gain::process()
{
	//const auto gain = getParam("gain")->getRaw();
	const auto gain = 1.0f;
	_audioBuffer.applyGain(gain);
}
