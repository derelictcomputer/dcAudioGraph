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
	lm->setNumIo(Audio | Input | Output, numIo);

	const auto gId = g.addModule(std::make_unique<Gain>());
	auto* gain = dynamic_cast<Gain*>(g.getModuleById(gId));
	ASSERT_NE(gain, nullptr);
	gain->setNumIo(Audio | Input | Output, numIo);

	g.setNumIo(Audio | Input | Output, numIo);
	EXPECT_EQ(g.getNumIo(Audio | Input), numIo);
	EXPECT_EQ(g.getNumIo(Audio | Output), numIo);

	auto* aIn = g.getInputModule();
	ASSERT_NE(aIn, nullptr);
	auto* aOut = g.getOutputModule();
	ASSERT_NE(aOut, nullptr);

	for (size_t cIdx = 0; cIdx < numIo; ++cIdx)
	{
		bool res;
		{
			res = g.addConnection({ aIn->getId(), cIdx, gain->getId(), cIdx, Connection::Type::Audio });
			EXPECT_TRUE(res);
		}
		{
			res = g.addConnection({ gain->getId(), cIdx, lm->getId(), cIdx, Connection::Type::Audio });
			EXPECT_TRUE(res);
		}
		{
			res = g.addConnection({ lm->getId(), cIdx, aOut->getId(), cIdx, Connection::Type::Audio });
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

	AudioBuffer controlBuffer;
	ControlBuffer eventBuffer;

	Graph g;
	makeBasicGraph(g, numIo);
	g.setBlockSize(numSamples);
	g.setSampleRate(44100);
	g.process(outBuffer, controlBuffer, eventBuffer);
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

	AudioBuffer controlBuffer;
	ControlBuffer eventBuffer;

	Graph g;
	makeBasicGraph(g, numIo);
	g.setBlockSize(numSamples);
	g.setSampleRate(44100);

	for (int i = 0; i < 1000; ++i)
	{
		g.process(outBuffer, controlBuffer, eventBuffer);
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
	ASSERT_EQ(g.getNumIo(Audio | Input), 0);
	ASSERT_EQ(g.getNumIo(Audio | Output), 0);
	ASSERT_EQ(g.getNumIo(Control | Input), 0);
	ASSERT_EQ(g.getNumIo(Control | Output), 0);
	{
		const size_t numAIn = 5;
		const size_t numAOut = 13;
		const size_t numCIn = 7;
		const size_t numCOut = 3;
		auto* in = g.getInputModule();
		auto* out = g.getOutputModule();
		g.setNumIo(Audio | Input, numAIn);
		ASSERT_EQ(g.getNumIo(Audio | Input), numAIn);
		ASSERT_EQ(g.getNumIo(Audio | Output), 0);
		ASSERT_EQ(in->getNumIo(Audio | Output), numAIn);
		ASSERT_EQ(out->getNumIo(Audio | Input), 0);
		g.setNumIo(Audio | Output, numAOut);
		ASSERT_EQ(g.getNumIo(Audio | Input), numAIn);
		ASSERT_EQ(g.getNumIo(Audio | Output), numAOut);
		ASSERT_EQ(in->getNumIo(Audio | Output), numAIn);
		ASSERT_EQ(out->getNumIo(Audio | Input), numAOut);
		g.setNumIo(Control | Input, numCIn);
		ASSERT_EQ(g.getNumIo(Control | Input), numCIn);
		ASSERT_EQ(g.getNumIo(Control | Output), 0);
		ASSERT_EQ(in->getNumIo(Control | Output), numCIn);
		ASSERT_EQ(out->getNumIo(Control | Input), 0);
		g.setNumIo(Control | Output, numCOut);
		ASSERT_EQ(g.getNumIo(Control | Input), numCIn);
		ASSERT_EQ(g.getNumIo(Control | Output), numCOut);
		ASSERT_EQ(in->getNumIo(Control | Output), numCIn);
		ASSERT_EQ(out->getNumIo(Control | Input), numCOut);
	}
}

TEST(Graph, Connections)
{
	Graph g;
	ASSERT_EQ(g.getNumConnections(), 0);
	auto* in = g.getInputModule();
	auto id = g.addModule(std::make_unique<Gain>());
	auto res = g.addConnection({ in->getId(), 0, id, 0, Connection::Type::Audio });
	ASSERT_EQ(res, false);
	ASSERT_EQ(g.getNumConnections(), 0);
	g.setNumIo(Audio | Input, 2);
	auto* gain = dynamic_cast<Gain*>(g.getModuleById(id));
	gain->setNumIo(Audio | Input, 5);
	res = g.addConnection({ in->getId(), 0, id, 4, Connection::Type::Audio });
	ASSERT_EQ(res, true);
	ASSERT_EQ(g.getNumConnections(), 1);
	g.removeConnection({ in->getId(), 0, id, 4, Connection::Type::Audio });
	ASSERT_EQ(g.getNumConnections(), 0);
	res = g.addConnection({ in->getId(), 0, id, 0, Connection::Type::Control });
	ASSERT_EQ(res, false);
	ASSERT_EQ(g.getNumConnections(), 0);
	res = g.addConnection({ in->getId(), 0, id, 4, Connection::Type::Audio });
	ASSERT_EQ(res, true);
	ASSERT_EQ(g.getNumConnections(), 1);
	g.setNumIo(Control | Input, 3);
	id = g.addModule(std::make_unique<Graph>());
	auto* graph = dynamic_cast<Graph*>(g.getModuleById(id));
	graph->setNumIo(Control | Input, 9);
	res = g.addConnection({ in->getId(), 1, id, 7, Connection::Type::Control });
	ASSERT_EQ(res, true);
	ASSERT_EQ(g.getNumConnections(), 2);
	g.disconnectModule(id);
	ASSERT_EQ(g.getNumConnections(), 1);
	g.disconnectModule(in->getId());
	ASSERT_EQ(g.getNumConnections(), 0);
}
