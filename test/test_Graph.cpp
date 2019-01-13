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

TEST(Graph, PassthroughBasic)
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

TEST(Graph, PassthroughBasicLoop)
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

TEST(Graph, AddRemove)
{
	Graph g;
	ASSERT_NE(g.getInputModule(), nullptr);
	ASSERT_NE(g.getOutputModule(), nullptr);
	ASSERT_EQ(g.getNumModules(), 0);
	auto id = g.addModule(std::make_unique<LevelMeter>());
	ASSERT_EQ(g.getNumModules(), 1);
	ASSERT_EQ(id, 3);
	ASSERT_NE(g.getModuleById(id), nullptr);
	g.removeModuleById(id);
	ASSERT_EQ(g.getNumModules(), 0);
	ASSERT_EQ(g.getModuleById(id), nullptr);
}

TEST(Graph, SetNumIo)
{
	Graph g;
	ASSERT_EQ(g.getNumAudioInputs(), 0);
	ASSERT_EQ(g.getNumAudioOutputs(), 0);
	ASSERT_EQ(g.getNumControlInputs(), 0);
	ASSERT_EQ(g.getNumControlOutputs(), 0);
	{
		const size_t numAIn = 5;
		const size_t numAOut = 17;
		const size_t numCIn = 99;
		const size_t numCOut = 3;
		auto* in = g.getInputModule();
		auto* out = g.getOutputModule();
		g.setNumAudioInputs(numAIn);
		ASSERT_EQ(g.getNumAudioInputs(), numAIn);
		ASSERT_EQ(in->getNumAudioOutputs(), numAIn);
		ASSERT_EQ(out->getNumAudioInputs(), 0);
		g.setNumAudioOutputs(numAOut);
		ASSERT_EQ(g.getNumAudioOutputs(), numAOut);
		ASSERT_EQ(out->getNumAudioInputs(), numAOut);
		ASSERT_EQ(in->getNumAudioOutputs(), numAIn);
		g.setNumControlInputs(numCIn);
		ASSERT_EQ(g.getNumControlInputs(), numCIn);
		ASSERT_EQ(in->getNumControlOutputs(), numCIn);
		ASSERT_EQ(out->getNumControlInputs(), 0);
		g.setNumControlOutputs(numCOut);
		ASSERT_EQ(g.getNumControlOutputs(), numCOut);
		ASSERT_EQ(out->getNumControlInputs(), numCOut);
		ASSERT_EQ(in->getNumControlOutputs(), numCIn);
	}
	{
		g.setNumAudioInputs(0);
		g.setNumAudioOutputs(0);
		g.setNumControlInputs(0);
		g.setNumControlOutputs(0);
		ASSERT_EQ(g.getNumAudioInputs(), 0);
		ASSERT_EQ(g.getNumAudioOutputs(), 0);
		ASSERT_EQ(g.getNumControlInputs(), 0);
		ASSERT_EQ(g.getNumControlOutputs(), 0);
	}
	{
		const size_t numAIn = 18;
		const size_t numAOut = 57;
		const size_t numCIn = 12;
		const size_t numCOut = 9;
		auto* in = g.getInputModule();
		auto* out = g.getOutputModule();
		g.setNumAudioInputs(numAIn);
		ASSERT_EQ(g.getNumAudioInputs(), numAIn);
		ASSERT_EQ(in->getNumAudioOutputs(), numAIn);
		ASSERT_EQ(out->getNumAudioInputs(), 0);
		g.setNumAudioOutputs(numAOut);
		ASSERT_EQ(g.getNumAudioOutputs(), numAOut);
		ASSERT_EQ(out->getNumAudioInputs(), numAOut);
		ASSERT_EQ(in->getNumAudioOutputs(), numAIn);
		g.setNumControlInputs(numCIn);
		ASSERT_EQ(g.getNumControlInputs(), numCIn);
		ASSERT_EQ(in->getNumControlOutputs(), numCIn);
		ASSERT_EQ(out->getNumControlInputs(), 0);
		g.setNumControlOutputs(numCOut);
		ASSERT_EQ(g.getNumControlOutputs(), numCOut);
		ASSERT_EQ(out->getNumControlInputs(), numCOut);
		ASSERT_EQ(in->getNumControlOutputs(), numCIn);
	}
}

TEST(Graph, Connections)
{
	Graph g;
	ASSERT_EQ(g.getNumConnections(), 0);
	auto* in = g.getInputModule();
	auto id = g.addModule(std::make_unique<Gain>());
	auto res = g.addConnection({ in->getId(), 0, id, 0, Module::Connection::Audio });
	ASSERT_EQ(res, false);
	ASSERT_EQ(g.getNumConnections(), 0);
	g.setNumAudioInputs(1);
	auto* gain = dynamic_cast<Gain*>(g.getModuleById(id));
	gain->setNumChannels(5);
	res = g.addConnection({ in->getId(), 0, id, 4, Module::Connection::Audio });
	ASSERT_EQ(res, true);
	ASSERT_EQ(g.getNumConnections(), 1);
	g.removeConnection({ in->getId(), 0, id, 4, Module::Connection::Audio });
	ASSERT_EQ(g.getNumConnections(), 0);
	res = g.addConnection({ in->getId(), 0, id, 0, Module::Connection::Control });
	ASSERT_EQ(res, false);
	ASSERT_EQ(g.getNumConnections(), 0);
	res = g.addConnection({ in->getId(), 0, id, 4, Module::Connection::Audio });
	ASSERT_EQ(res, true);
	ASSERT_EQ(g.getNumConnections(), 1);
	g.setNumControlInputs(3);
	id = g.addModule(std::make_unique<Graph>());
	auto* graph = dynamic_cast<Graph*>(g.getModuleById(id));
	graph->setNumControlInputs(9);
	res = g.addConnection({ in->getId(), 1, id, 7, Module::Connection::Control });
	ASSERT_EQ(res, true);
	ASSERT_EQ(g.getNumConnections(), 2);
	g.disconnectModule(id);
	ASSERT_EQ(g.getNumConnections(), 1);
	g.disconnectModule(in->getId());
	ASSERT_EQ(g.getNumConnections(), 0);
}
