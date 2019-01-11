#include "LevelMeter.h"

float dc::LevelMeter::getLevel(size_t channel)
{
	if (channel < _levels.size())
	{
		return _levels[channel];
	}
	return 0.0f;
}

void dc::LevelMeter::onProcess()
{
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

void dc::LevelMeter::onRefreshAudioBuffers()
{
	const auto numChannels = _audioBuffer.getNumChannels();
	while (numChannels < _levels.size())
	{
		_levels.pop_back();
	}
	while (numChannels > _levels.size())
	{
		_levels.emplace_back(0.0f);
	}
}
