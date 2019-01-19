#include "LevelMeter.h"

dc::LevelMeter::LevelMeter()
{
	_levels.reserve(maxAudioChannels);
}

float dc::LevelMeter::getLevel(size_t channel)
{
	if (channel < _levels.size())
	{
		return _levels[channel];
	}
	return 0.0f;
}

void dc::LevelMeter::setNumAudioIo(size_t num, bool isInput)
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

void dc::LevelMeter::process()
{
	const size_t nChannels = _audioBuffer.getNumChannels();

	while (nChannels < _levels.size())
	{
		_levels.pop_back();
	}
	while (nChannels > _levels.size())
	{
		_levels.emplace_back(0.0f);
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
