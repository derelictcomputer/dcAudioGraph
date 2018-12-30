#include "gtest/gtest.h"
#include "../dcAudioGraph/AudioBuffer.h"
#include "Test_Common.h"

using namespace dc;

TEST(AudioBuffer, ResizeFunctional)
{
	const size_t numSamples = 512;
	const size_t numChannels = 8;
	AudioBuffer b1;
	EXPECT_EQ(b1.getNumSamples(), 0);
	EXPECT_EQ(b1.getNumChannels(), 0);
	b1.resize(numSamples, numChannels);
	ASSERT_EQ(b1.getNumSamples(), numSamples);
	ASSERT_EQ(b1.getNumChannels(), numChannels);
}

TEST(AudioBuffer, ZeroFunctional)
{
	const size_t numSamples = 512;
	const size_t numChannels = 4;
	AudioBuffer b1;
	makeTestBuffer(b1, numSamples, numChannels, 0.0f);
	AudioBuffer b2;
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
	AudioBuffer b1;
	makeTestBuffer(b1, numSamples, numChannels, 0.0f);
	AudioBuffer b2;
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
	AudioBuffer b1;
	makeTestBuffer(b1, numSamples, numChannels, 0.0f);
	AudioBuffer b2;
	makeTestBuffer(b2, numSamples, numChannels, 1.0f);

	for (size_t cIdx = 0; cIdx < b2.getNumChannels(); ++cIdx)
	{
		b2.copyFrom(b1, cIdx, cIdx);
	}

	ASSERT_TRUE(buffersEqual(b1, b2));
}

TEST(AudioBuffer, FromInterleavedFunctional)
{
	const size_t numSamples = 512;
	const size_t numChannels = 4;

	std::vector<float> interleaved;
	interleaved.resize(numSamples * numChannels);
	for (size_t sIdx = 0; sIdx < numSamples; ++sIdx)
	{
		for (size_t cIdx = 0; cIdx < numChannels; ++cIdx)
		{
			interleaved[cIdx + sIdx * numChannels] = cIdx;
		}
	}

	// with resize
	{
		AudioBuffer b;
		AudioBuffer expected;
		expected.resize(numSamples, numChannels);
		for (size_t cIdx = 0; cIdx < numChannels; ++cIdx)
		{
			auto* cPtr = expected.getChannelPointer(cIdx);
			for (size_t sIdx = 0; sIdx < numSamples; ++sIdx)
			{
				cPtr[sIdx] = cIdx;
			}
		}
		b.fromInterleaved(interleaved.data(), numSamples, numChannels, true);
		ASSERT_TRUE(buffersEqual(b, expected));
	}

	// without resize
	{
		const size_t noResizeNumSamples = 64;
		const size_t noResizeNumChannels = 8;
		AudioBuffer b;
		b.resize(noResizeNumSamples, noResizeNumChannels);
		AudioBuffer expected;
		expected.resize(noResizeNumSamples, noResizeNumChannels);
		for (size_t cIdx = 0; cIdx < numChannels; ++cIdx)
		{
			if (cIdx >= noResizeNumChannels)
			{
				break;
			}
			auto* cPtr = expected.getChannelPointer(cIdx);
			for (size_t sIdx = 0; sIdx < numSamples; ++sIdx)
			{
				if (sIdx >= noResizeNumSamples)
				{
					break;
				}
				cPtr[sIdx] = cIdx;
			}
		}
		b.fromInterleaved(interleaved.data(), numSamples, numChannels, false);
		ASSERT_TRUE(buffersEqual(b, expected));
	}
}

TEST(AudioBuffer, ToInterleavedFunctional)
{
	const size_t numSamples = 256;
	const size_t numChannels = 6;

	AudioBuffer b;
	b.resize(numSamples, numChannels);
	for (size_t cIdx = 0; cIdx < numChannels; ++cIdx)
	{
		auto* cPtr = b.getChannelPointer(cIdx);
		for (size_t sIdx = 0; sIdx < numSamples; ++sIdx)
		{
			cPtr[sIdx] = cIdx;
		}
	}

	std::vector<float> expected;
	expected.resize(numSamples * numChannels);
	for (size_t sIdx = 0; sIdx < numSamples; ++sIdx)
	{
		for (size_t cIdx = 0; cIdx < numChannels; ++cIdx)
		{
			expected[cIdx + sIdx * numChannels] = cIdx;
		}
	}

	std::vector<float> testBuffer;
	testBuffer.resize(numChannels * numSamples);

	b.toInterleaved(testBuffer.data(), numSamples, numChannels);

	for (size_t sIdx = 0; sIdx < numSamples; ++sIdx)
	{
		for (size_t cIdx = 0; cIdx < numChannels; ++cIdx)
		{
			const float sTest = testBuffer[cIdx + sIdx * numChannels];
			const float sExpected = expected[cIdx + sIdx * numChannels];
			ASSERT_TRUE(samplesEqual(sTest, sExpected));
		}
	}
}
