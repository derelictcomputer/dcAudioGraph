#include "LevelMeter.h"

dc::LevelMeter::LevelMeterProcessor::LevelMeterProcessor(LevelMeter& parent) : _parent(parent)
{
}

void dc::LevelMeter::LevelMeterProcessor::process(AudioBuffer& audioBuffer, ControlBuffer& /*controlBuffer*/)
{
	if (_parent.wantsMessage())
	{
		for (size_t cIdx = 0; cIdx < audioBuffer.getNumChannels(); ++cIdx)
		{
			auto* cPtr = audioBuffer.getChannelPointer(cIdx);
			float sum = 0.0f;
			for (size_t sIdx = 0; sIdx < audioBuffer.getNumSamples(); ++sIdx)
			{
				const float sample = cPtr[sIdx];
				sum += sample * sample;
			}
			const auto level = std::sqrt(sum / audioBuffer.getNumSamples());
			LevelMessage msg{};
			msg.index = cIdx;
			msg.level = level;
			_parent.pushLevelMessage(msg);
		}
	}
}

dc::LevelMeter::LevelMeter() :
	Module(std::make_unique<LevelMeterProcessor>(*this)),
	_levelMessageQueue(MODULE_DEFAULT_MAX_IO)
{
	setNumIo(Audio | Input | Output, 1);
	_levels.resize(MODULE_DEFAULT_MAX_IO);
}

float dc::LevelMeter::getLevel(size_t channel)
{
	if (channel < _levels.size())
	{
		return _levels[channel];
	}
	return 0.0f;
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
