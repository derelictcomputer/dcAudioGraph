#include "gtest/gtest.h"
#include "Test_Common.h"
#include "../dcAudioGraph/LevelMeter.h"
#include "../dcAudioGraph/Gain.h"

using namespace dc;

void makeBasicGraph(Graph& g, size_t numIo)
{
	auto lmId = g.addModule(std::make_unique<LevelMeter>());
	auto* lm = dynamic_cast<LevelMeter*>(g.getModuleById(lmId));
	ASSERT_NE(lm, nullptr);
	lm->setNumChannels(numIo);

	auto gId = g.addModule(std::make_unique<Gain>());
	auto* gain = dynamic_cast<Gain*>(g.getModuleById(gId));
	ASSERT_NE(gain, nullptr);
	gain->setNumChannels(numIo);

	g.setNumAudioInputs(numIo);
	EXPECT_EQ(g.getNumAudioInputs(), numIo);
	g.setNumAudioOutputs(numIo);
	EXPECT_EQ(g.getNumAudioOutputs(), numIo);

	auto* aIn = g.getInputModule();
	ASSERT_NE(aIn, nullptr);
	auto* aOut = g.getOutputModule();
	ASSERT_NE(aOut, nullptr);

	for (size_t cIdx = 0; cIdx < numIo; ++cIdx)
	{
		bool res;
		{
			res = g.addConnection({ aIn->getId(), cIdx, gain->getId(), cIdx, Module::Connection::Audio });
			EXPECT_TRUE(res);
		}
		{
			res = g.addConnection({ gain->getId(), cIdx, lm->getId(), cIdx, Module::Connection::Audio });
			EXPECT_TRUE(res);
		}
		{
			res = g.addConnection({ lm->getId(), cIdx, aOut->getId(), cIdx, Module::Connection::Audio });
			EXPECT_TRUE(res);
		}
	}
}

TEST(Graph, GraphIOBasic)
{
	const size_t numSamples = 512;
	const size_t numIo = 1;
	const float testValue = 1.0f;

	AudioBuffer inBuffer(numSamples, numIo);
	inBuffer.fill(testValue);
	AudioBuffer outBuffer;
	outBuffer.copyFrom(inBuffer, true);
	EXPECT_TRUE(buffersEqual(inBuffer, outBuffer));

	ControlBuffer controlBuffer;

	Graph g;
	makeBasicGraph(g, numIo);
	g.setBlockSize(numSamples);
	g.setSampleRate(44100);
	g.process(outBuffer, controlBuffer);
	ASSERT_TRUE(buffersEqual(inBuffer, outBuffer));
}

TEST(Graph, GraphIOBasic_Loop)
{
	const size_t numSamples = 512;
	const size_t numIo = 1;
	const float testValue = 1.0f;

	AudioBuffer inBuffer(numSamples, numIo);
	inBuffer.fill(testValue);
	AudioBuffer outBuffer;
	outBuffer.copyFrom(inBuffer, true);
	EXPECT_TRUE(buffersEqual(inBuffer, outBuffer));

	ControlBuffer controlBuffer;

	Graph g;
	makeBasicGraph(g, numIo);
	g.setBlockSize(numSamples);
	g.setSampleRate(44100);

	for (int i = 0; i < 1000; ++i)
	{
		g.process(outBuffer, controlBuffer);
		ASSERT_TRUE(buffersEqual(inBuffer, outBuffer));
	}
}
