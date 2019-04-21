#include "Test_Common.h"
#include <cmath>

bool dc::samplesEqual(float s0, float s1)
{
  return std::abs(s0 - s1) < 0.0001f;
}

bool dc::buffersEqual(AudioBuffer& b0, AudioBuffer& b1)
{
  if (b0.getNumChannels() != b1.getNumChannels())
  {
    return false;
  }

  if (b0.getNumSamples() != b1.getNumSamples())
  {
    return false;
  }

  for (size_t cIdx = 0; cIdx < b0.getNumChannels(); ++cIdx)
  {
    auto* b0Ptr = b0.getChannelPointer(cIdx);
    auto* b1Ptr = b1.getChannelPointer(cIdx);

    if (nullptr == b0Ptr && nullptr == b1Ptr)
    {
      return true;
    }

    for (size_t sIdx = 0; sIdx < b0.getNumSamples(); ++sIdx)
    {
      const float b0Sample = b0Ptr[sIdx];
      const float b1Sample = b1Ptr[sIdx];
      if (!samplesEqual(b0Sample, b1Sample))
      {
        return false;
      }
    }
  }

  return true;
}
