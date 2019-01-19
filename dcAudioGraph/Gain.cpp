#include "Gain.h"

dc::Gain::Gain()
{
	addParam("gain", "Gain", ParamRange(0.0f, 2.0f, 0.0f), true, true);
	getParamById("gain")->setRaw(1.0f);
}

void dc::Gain::setNumAudioIo(size_t num, bool isInput)
{
	while (num < getNumAudioIo(true))
	{
		removeAudioIo(getNumAudioIo(true) - 1, true);
		removeAudioIo(getNumAudioIo(false) - 1, false);
	}
	while (num > getNumAudioIo(true))
	{
		addAudioIo(true);
		addAudioIo(false);
	}
}

void dc::Gain::process()
{
	const auto gain = getParamAt(0)->getRaw();
	_audioBuffer.applyGain(gain);
}
