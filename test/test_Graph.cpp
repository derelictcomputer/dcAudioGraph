#include "gtest/gtest.h"
#include "../dcAudioGraph/Graph.h"
#include "Test_Common.h"

using namespace dc;

void makeBasicGraph(Graph& g, size_t numIo)
{
	g.setNumAudioInputs(numIo);
	EXPECT_EQ(g.getNumAudioInputs(), numIo);
	g.setNumAudioOutputs(numIo);
	EXPECT_EQ(g.getNumAudioOutputs(), numIo);

	const auto ptId = g.addModule(std::make_unique<dc::GraphOutputModule>());
	EXPECT_GT(ptId, 0);

	auto* pt = g.getModuleById(ptId);
	ASSERT_NE(pt, nullptr);

	pt->setNumAudioInputs(numIo);
	EXPECT_EQ(pt->getNumAudioInputs(), numIo);
	pt->setNumAudioOutputs(numIo);
	EXPECT_EQ(pt->getNumAudioOutputs(), numIo);

	auto* aIn = g.getAudioInput();
	ASSERT_NE(aIn, nullptr);
	auto* aOut = g.getAudioOutput();
	ASSERT_NE(aOut, nullptr);

	for (size_t cIdx = 0; cIdx < numIo; ++cIdx)
	{
		EXPECT_TRUE(dc::Module::connectAudio(aIn, cIdx, pt, cIdx));
		EXPECT_TRUE(dc::Module::connectAudio(pt, cIdx, aOut, cIdx));
	}
}

TEST(Graph, GraphIOBasic)
{
	const size_t numSamples = 512;
	const size_t numIo = 1;
	const float testValue = 1.0f;

	AudioBuffer inBuffer(numSamples, numIo);
	inBuffer.fill(testValue);
	AudioBuffer outBuffer(numSamples, numIo);

	Graph g;
	makeBasicGraph(g, numIo);
	g.init(numSamples, 44100);
	g.process(inBuffer, outBuffer);
	ASSERT_TRUE(buffersEqual(inBuffer, outBuffer));
}

TEST(Graph, GraphIOBasic_Loop)
{
	const size_t numSamples = 512;
	const size_t numIo = 1;
	const float testValue = 1.0f;

	AudioBuffer inBuffer(numSamples, numIo);
	inBuffer.fill(testValue);
	AudioBuffer outBuffer(numSamples, numIo);

	Graph g;
	makeBasicGraph(g, numIo);
	g.init(numSamples, 44100);

	for (int i = 0; i < 1000; ++i)
	{
		g.process(inBuffer, outBuffer);
		ASSERT_TRUE(buffersEqual(inBuffer, outBuffer));
	}
}
