/*
 * Provides level readings for the most recent buffer,
 * and passes through audio and control.
 */

#pragma once

#include "Module.h"
#include "MessageQueue.h"

namespace dc
{
class LevelMeter : public Module
{
public:
  LevelMeter();

  float getLevel(size_t channel);

protected:
  struct LevelMessage
  {
    size_t index;
    float level;
  };

  void process(ModuleProcessContext& context) override;

  bool wantsMessage() const { return _levelMessageQueue.empty(); }

  bool pushLevelMessage(const LevelMessage& msg);

  void handleLevelMessages();

  MessageQueue<LevelMessage> _levelMessageQueue;
  std::vector<float> _levels;
};
}
