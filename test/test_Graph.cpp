#include "gtest/gtest.h"
#include "Test_Common.h"
#include "../dcAudioGraph/LevelMeter.h"
#include "../dcAudioGraph/Gain.h"

using namespace dc;

void makeBasicGraph(Graph& g, size_t numIo)
{
	const auto lmId = g.addModule(std::make_unique<LevelMeter>());
	auto* lm = dynamic_cast<LevelMeter*>(g.getModuleById(lmId));
	ASSERT_NE(lm, nullptr);
	lm->setNumAudioIo(numIo, true);

	const auto gId = g.addModule(std::make_unique<Gain>());
	auto* gain = dynamic_cast<Gain*>(g.getModuleById(gId));
	ASSERT_NE(gain, nullptr);
	gain->setNumAudioIo(numIo, true);

	g.setNumAudioIo(numIo, true);
	EXPECT_EQ(g.getNumAudioIo(true), numIo);
	g.setNumAudioIo(numIo, false);
	EXPECT_EQ(g.getNumAudioIo(false), numIo);

	auto* aIn = g.getInputModule();
	ASSERT_NE(aIn, nullptr);
	auto* aOut = g.getOutputModule();
	ASSERT_NE(aOut, nullptr);

	for (size_t cIdx = 0; cIdx < numIo; ++cIdx)
	{
		bool res;
		{
			res = g.addConnection({ aIn->getId(), cIdx, gain->getId(), cIdx, Graph::Connection::Audio });
			EXPECT_TRUE(res);
		}
		{
			res = g.addConnection({ gain->getId(), cIdx, lm->getId(), cIdx, Graph::Connection::Audio });
			EXPECT_TRUE(res);
		}
		{
			res = g.addConnection({ lm->getId(), cIdx, aOut->getId(), cIdx, Graph::Connection::Audio });
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
	ASSERT_EQ(g.getNumAudioIo(true), 0);
	ASSERT_EQ(g.getNumAudioIo(false), 0);
	ASSERT_EQ(g.getNumControlIo(true), 0);
	ASSERT_EQ(g.getNumControlIo(false), 0);
	{
		const size_t numAIn = 5;
		const size_t numAOut = 17;
		const size_t numCIn = 99;
		const size_t numCOut = 3;
		auto* in = g.getInputModule();
		auto* out = g.getOutputModule();
		g.setNumAudioIo(numAIn, true);
		ASSERT_EQ(g.getNumAudioIo(true), numAIn);
		ASSERT_EQ(in->getNumAudioIo(false), numAIn);
		ASSERT_EQ(out->getNumAudioIo(true), 0);
		g.setNumAudioIo(numAOut, false);
		ASSERT_EQ(g.getNumAudioIo(false), numAOut);
		ASSERT_EQ(out->getNumAudioIo(true), numAOut);
		ASSERT_EQ(in->getNumAudioIo(false), numAIn);
		g.setNumControlIo(numCIn, true);
		ASSERT_EQ(g.getNumControlIo(true), numCIn);
		ASSERT_EQ(in->getNumControlIo(false), numCIn);
		ASSERT_EQ(out->getNumControlIo(true), 0);
		g.setNumControlIo(numCOut, false);
		ASSERT_EQ(g.getNumControlIo(false), numCOut);
		ASSERT_EQ(out->getNumControlIo(true), numCOut);
		ASSERT_EQ(in->getNumControlIo(false), numCIn);
	}
}

TEST(Graph, Connections)
{
	Graph g;
	ASSERT_EQ(g.getNumConnections(), 0);
	auto* in = g.getInputModule();
	auto id = g.addModule(std::make_unique<Gain>());
	auto res = g.addConnection({ in->getId(), 0, id, 0, Graph::Connection::Audio });
	ASSERT_EQ(res, false);
	ASSERT_EQ(g.getNumConnections(), 0);
	g.setNumAudioIo(2, true);
	auto* gain = dynamic_cast<Gain*>(g.getModuleById(id));
	gain->setNumAudioIo(5, true);
	res = g.addConnection({ in->getId(), 0, id, 4, Graph::Connection::Audio });
	ASSERT_EQ(res, true);
	ASSERT_EQ(g.getNumConnections(), 1);
	g.removeConnection({ in->getId(), 0, id, 4, Graph::Connection::Audio });
	ASSERT_EQ(g.getNumConnections(), 0);
	res = g.addConnection({ in->getId(), 0, id, 0, Graph::Connection::Control });
	ASSERT_EQ(res, false);
	ASSERT_EQ(g.getNumConnections(), 0);
	res = g.addConnection({ in->getId(), 0, id, 4, Graph::Connection::Audio });
	ASSERT_EQ(res, true);
	ASSERT_EQ(g.getNumConnections(), 1);
	g.setNumControlIo(3, true);
	id = g.addModule(std::make_unique<Graph>());
	auto* graph = dynamic_cast<Graph*>(g.getModuleById(id));
	graph->setNumControlIo(9, true);
	res = g.addConnection({ in->getId(), 1, id, 7, Graph::Connection::Control });
	ASSERT_EQ(res, true);
	ASSERT_EQ(g.getNumConnections(), 2);
	g.disconnectModule(id);
	ASSERT_EQ(g.getNumConnections(), 1);
	g.disconnectModule(in->getId());
	ASSERT_EQ(g.getNumConnections(), 0);
}
