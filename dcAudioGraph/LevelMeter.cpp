#include "LevelMeter.h"
#include <cmath>

dc::LevelMeter::LevelMeter() :
    _levelMessageQueue(MODULE_DEFAULT_MAX_IO)
{
  setNumIo(Audio | Input | Output, 1);
  _levels.resize(MODULE_DEFAULT_MAX_IO);
  addParam("type", "Type", ParamRange(0, 1, 1), true, false, 0);
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
    const size_t nChannels = context.audioBuffer.getNumChannels();
    const int type = (int)context.params[0]->getRaw();

    for (size_t cIdx = 0; cIdx < nChannels; ++cIdx)
    {
      const float level = type == 0
                          ? context.audioBuffer.getPeak(cIdx)
                          : context.audioBuffer.getRms(cIdx);
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
