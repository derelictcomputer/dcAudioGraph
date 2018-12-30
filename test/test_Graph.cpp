#include "gtest/gtest.h"
#include "../dcAudioGraph/Graph.h"
#include "Test_Common.h"

void makeBasicGraph(dc::Graph& g, size_t numIo)
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

	dc::AudioBuffer inBuffer;
	makeTestBuffer(inBuffer, numSamples, numIo, testValue);
	dc::AudioBuffer outBuffer;
	makeTestBuffer(outBuffer, numSamples, numIo, 0.0f);

	dc::Graph g;
	makeBasicGraph(g, 1);
	g.init(numSamples);
	g.process(inBuffer, outBuffer);
	ASSERT_TRUE(buffersEqual(inBuffer, outBuffer));
}

TEST(Graph, GraphIOBasic_Loop)
{
	const size_t numSamples = 512;
	const size_t numIo = 1;
	const float testValue = 1.0f;

	dc::AudioBuffer inBuffer;
	makeTestBuffer(inBuffer, numSamples, numIo, testValue);
	dc::AudioBuffer outBuffer;
	makeTestBuffer(outBuffer, numSamples, numIo, 0.0f);

	dc::Graph g;
	makeBasicGraph(g, 1);
	g.init(numSamples);

	for (int i = 0; i < 1000; ++i)
	{
		g.process(inBuffer, outBuffer);
		ASSERT_TRUE(buffersEqual(inBuffer, outBuffer));
	}
}
