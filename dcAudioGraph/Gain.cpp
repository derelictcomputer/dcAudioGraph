#include "Gain.h"

dc::Gain::Gain()
{
	addParam("nChannels", "# Channels", ParamRange(1, 16, 1), true, false);
	getParamAt(0)->setRaw(1);
	addParam("gain", "Gain", ParamRange(0.0f, 2.0f, 0.0f), true, true);
	getParamById("gain")->setRaw(1.0f);
}

void dc::Gain::process()
{
	const size_t nChannels = static_cast<size_t>(getParamAt(0)->getRaw());
	while (nChannels < getNumAudioIo(true))
	{
		const size_t idx = getNumAudioIo(true) - 1;
		removeAudioIo(idx, true);
		removeAudioIo(idx, false);
	}
	while (nChannels > getNumAudioIo(true))
	{
		addAudioIo(true);
		addAudioIo(false);
	}

	const auto gain = getParamAt(0)->getRaw();
	_audioBuffer.applyGain(gain);
}
