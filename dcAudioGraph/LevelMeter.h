/*
 * Provides simple RMS readings for the most recent buffer,
 * and passes through audio and control.
 */

#pragma once

#include "Module.h"

namespace dc
{
class LevelMeter : public Module
{
public:
	struct LevelMessage
	{
		size_t index;
		float level;
	};

	class LevelMeterProcessor : public ModuleProcessor
	{
	public:
		LevelMeterProcessor(LevelMeter& parent);

	protected:
		void process(AudioBuffer& audioBuffer, ControlBuffer& controlBuffer) override;

		LevelMeter& _parent;
	};

	LevelMeter();

	float getLevel(size_t channel);

	bool wantsMessage() const { return _levelMessageQueue.empty(); }
	bool pushLevelMessage(const LevelMessage& msg);

protected:
	void handleLevelMessages();

	MessageQueue<LevelMessage> _levelMessageQueue;
	std::vector<float> _levels;
};
}
