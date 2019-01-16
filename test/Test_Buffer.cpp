#include "gtest/gtest.h"
#include "Test_Common.h"

using namespace dc;

TEST(AudioBuffer, CopyConstructor)
{
	AudioBuffer b1(1024, 16);
	b1.fill(0.5f);
	AudioBuffer b2(b1);
	ASSERT_TRUE(buffersEqual(b1, b2));
}

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

TEST(AudioBuffer, FillFunctional)
{
	const size_t numSamples = 512;
	const size_t numChannels = 3;
	const float value = 0.5f;

	AudioBuffer b1(numSamples, numChannels);

	// manually fill the buffer, so we're not just checking if the thing's consistently wrong
	for (size_t cIdx = 0; cIdx < numChannels; ++cIdx)
	{
		auto* cPtr = b1.getChannelPointer(cIdx);
		for (size_t sIdx = 0; sIdx < numSamples; ++sIdx)
		{
			cPtr[sIdx] = value;
		}
	}

	AudioBuffer b2(numSamples, numChannels);
	b2.fill(value);

	ASSERT_TRUE(buffersEqual(b1, b2));

	for (size_t cIdx = 0; cIdx < numChannels; ++cIdx)
	{
		b1.fill(cIdx, 0.0f);
		ASSERT_FALSE(buffersEqual(b1, b2));
		b2.fill(cIdx, 0.0f);
		ASSERT_TRUE(buffersEqual(b1, b2));
	}
}

TEST(AudioBuffer, ZeroFunctional)
{
	const size_t numSamples = 512;
	const size_t numChannels = 3;

	AudioBuffer b1(numSamples, numChannels);
	// manually zero the buffer, so we're not just checking if the thing's consistently wrong
	for (size_t cIdx = 0; cIdx < numChannels; ++cIdx)
	{
		auto* cPtr = b1.getChannelPointer(cIdx);
		for (size_t sIdx = 0; sIdx < numSamples; ++sIdx)
		{
			cPtr[sIdx] = 0.0f;
		}
	}

	AudioBuffer b2(numSamples, numChannels);
	b2.fill(0.9f);

	EXPECT_FALSE(buffersEqual(b1, b2));

	b2.zero();

	ASSERT_TRUE(buffersEqual(b1, b2));

	b1.fill(0.2f);
	b2.fill(0.2f);

	EXPECT_TRUE(buffersEqual(b1, b2));

	for (size_t cIdx = 0; cIdx < numChannels; ++cIdx)
	{
		b1.zero(cIdx);
		ASSERT_FALSE(buffersEqual(b1, b2));
		b2.zero(cIdx);
		ASSERT_TRUE(buffersEqual(b1, b2));
	}
}

TEST(AudioBuffer, CopyFunctional)
{
	const size_t numSamples = 512;
	const size_t numChannels = 4;

	AudioBuffer b1;
	AudioBuffer b2(numSamples, numChannels);
	b2.fill(1.0f);

	EXPECT_FALSE(buffersEqual(b1, b2));

	b1.copyFrom(b2, true);

	ASSERT_TRUE(buffersEqual(b1, b2));

	b1.zero();

	for (size_t cIdx = 0; cIdx < b1.getNumChannels(); ++cIdx)
	{
		b1.copyFrom(b2, cIdx, cIdx);
	}

	ASSERT_TRUE(buffersEqual(b1, b2));
}

TEST(AudioBuffer, AddFunctional)
{
	const size_t numSamples = 512;
	const size_t numChannels = 4;

	AudioBuffer b1(numSamples, numChannels);
	AudioBuffer b2(numSamples, numChannels);
	b2.fill(1.0f);

	EXPECT_FALSE(buffersEqual(b1, b2));

	b1.addFrom(b2);

	ASSERT_TRUE(buffersEqual(b1, b2));

	b1.zero();

	for (size_t cIdx = 0; cIdx < b1.getNumChannels(); ++cIdx)
	{
		b1.addFrom(b2, cIdx, cIdx);
	}

	ASSERT_TRUE(buffersEqual(b1, b2));
}

TEST(AudioBuffer, ApplyGainFunctional)
{
	const size_t numSamples = 1000;
	const size_t numChannels = 3;

	AudioBuffer b1(numSamples, numChannels);
	b1.zero();
	AudioBuffer b2(numSamples, numChannels);
	b2.fill(1.0f);

	EXPECT_FALSE(buffersEqual(b1, b2));

	b2.applyGain(0.0f);

	ASSERT_TRUE(buffersEqual(b1, b2));

	b1.fill(0.5f);
	b2.fill(1.0f);

	for (size_t cIdx = 0; cIdx < b1.getNumChannels(); ++cIdx)
	{
		b1.applyGain(cIdx, 2.0f);
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
			interleaved[cIdx + sIdx * numChannels] = static_cast<float>(cIdx);
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
				cPtr[sIdx] = static_cast<float>(cIdx);
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
				cPtr[sIdx] = static_cast<float>(cIdx);
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
			cPtr[sIdx] = static_cast<float>(cIdx);
		}
	}

	std::vector<float> expected;
	expected.resize(numSamples * numChannels);
	for (size_t sIdx = 0; sIdx < numSamples; ++sIdx)
	{
		for (size_t cIdx = 0; cIdx < numChannels; ++cIdx)
		{
			expected[cIdx + sIdx * numChannels] = static_cast<float>(cIdx);
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
