#include "LevelMeter.h"
#include <cmath>

dc::LevelMeter::LevelMeter() :
	_levelMessageQueue(MODULE_DEFAULT_MAX_IO)
{
	setNumIo(Audio | Input | Output, 1);
	_levels.resize(MODULE_DEFAULT_MAX_IO);
}

float dc::LevelMeter::getLevel(size_t channel)
{
	handleLevelMessages();

	if (channel < _levels.size())
	{
		return _levels[channel];
	}

	return 0.0f;
}

void dc::LevelMeter::process(ModuleProcessContext& context)
{
	if (wantsMessage())
	{
		const size_t nSamples = context.audioBuffer.getNumSamples();
		const size_t nChannels = context.audioBuffer.getNumChannels();

		for (size_t cIdx = 0; cIdx < nChannels; ++cIdx)
		{
			auto* cPtr = context.audioBuffer.getChannelPointer(cIdx);
			float sum = 0.0f;
			for (size_t sIdx = 0; sIdx < nSamples; ++sIdx)
			{
				const float sample = cPtr[sIdx];
				sum += sample * sample;
			}
			const auto level = std::sqrt(sum / nSamples);
			LevelMessage msg{};
			msg.index = cIdx;
			msg.level = level;
			pushLevelMessage(msg);
		}
	}
}

bool dc::LevelMeter::pushLevelMessage(const LevelMessage& msg)
{
	return _levelMessageQueue.push(msg);
}

void dc::LevelMeter::handleLevelMessages()
{
	LevelMessage msg{};
	while (_levelMessageQueue.pop(msg))
	{
		_levels[msg.index] = msg.level;
	}
}
