#include "LevelMeter.h"

dc::LevelMeter::LevelMeter()
{
	addParam("nChannels", "# Channels", ParamRange(1, 16, 1), true, false);
	getParamAt(0)->setRaw(1);
}

float dc::LevelMeter::getLevel(size_t channel)
{
	if (channel < _levels.size())
	{
		return _levels[channel];
	}
	return 0.0f;
}

void dc::LevelMeter::process()
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

	for (size_t cIdx = 0; cIdx < _audioBuffer.getNumChannels(); ++cIdx)
	{
		auto* cPtr = _audioBuffer.getChannelPointer(cIdx);
		float sum = 0.0f;
		for (size_t sIdx = 0; sIdx < _audioBuffer.getNumSamples(); ++sIdx)
		{
			const auto sample = cPtr[sIdx];
			sum += sample * sample;
		}
		_levels[cIdx] = std::sqrt(sum / _audioBuffer.getNumSamples());
	}
}
