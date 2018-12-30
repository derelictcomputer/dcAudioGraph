#include "gtest/gtest.h"
#include "../dcAudioGraph/AudioBuffer.h"
#include "Test_Common.h"

TEST(AudioBuffer, ZeroFunctional)
{
	const size_t numSamples = 512;
	const size_t numChannels = 4;
	dc::AudioBuffer b1;
	makeTestBuffer(b1, numSamples, numChannels, 0.0f);
	dc::AudioBuffer b2;
	makeTestBuffer(b2, numSamples, numChannels, 1.0f);

	for (size_t cIdx = 0; cIdx < b2.getNumChannels(); ++cIdx)
	{
		b2.zero(cIdx);
	}

	ASSERT_TRUE(buffersEqual(b1, b2));
}

TEST(AudioBuffer, AddFunctional)
{
	const size_t numSamples = 512;
	const size_t numChannels = 4;
	dc::AudioBuffer b1;
	makeTestBuffer(b1, numSamples, numChannels, 0.0f);
	dc::AudioBuffer b2;
	makeTestBuffer(b2, numSamples, numChannels, 1.0f);

	for (size_t cIdx = 0; cIdx < b1.getNumChannels(); ++cIdx)
	{
		b1.addFrom(b2, cIdx, cIdx);
	}

	ASSERT_TRUE(buffersEqual(b1, b2));
}

TEST(AudioBuffer, CopyFunctional)
{
	const size_t numSamples = 512;
	const size_t numChannels = 4;
	dc::AudioBuffer b1;
	makeTestBuffer(b1, numSamples, numChannels, 0.0f);
	dc::AudioBuffer b2;
	makeTestBuffer(b2, numSamples, numChannels, 1.0f);

	for (size_t cIdx = 0; cIdx < b2.getNumChannels(); ++cIdx)
	{
		b2.copyFrom(b1, cIdx, cIdx);
	}

	ASSERT_TRUE(buffersEqual(b1, b2));
}
