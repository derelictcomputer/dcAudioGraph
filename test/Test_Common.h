#pragma once
#include "../dcAudioGraph/AudioBuffer.h"

inline bool samplesEqual(float s0, float s1)
{
	return std::fabs(s0 - s1) < 0.0001f;
}

inline bool buffersEqual(dc::AudioBuffer& b0, dc::AudioBuffer& b1)
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
		for (size_t sIdx = 0; sIdx < b0.getNumSamples(); ++sIdx)
		{
			if (!samplesEqual(b0Ptr[sIdx], b1Ptr[sIdx]))
			{
				return false;
			}
		}
	}

	return true;
}

inline void makeTestBuffer(dc::AudioBuffer& b, size_t numSamples, size_t numChannels, float value)
{
	b.resize(numSamples, numChannels);
	for (size_t cIdx = 0; cIdx < b.getNumChannels(); ++cIdx)
	{
		auto* cPtr = b.getChannelPointer(cIdx);
		for (size_t sIdx = 0; sIdx < b.getNumSamples(); ++sIdx)
		{
			cPtr[sIdx] = value;
		}
	}
}
