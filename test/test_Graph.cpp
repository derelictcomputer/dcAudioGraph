#include "gtest/gtest.h"
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

	auto* aIn = g.getInputModule();
	ASSERT_NE(aIn, nullptr);
	auto* aOut = g.getOutputModule();
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
	AudioBuffer outBuffer;
	outBuffer.copyFrom(inBuffer, true);
	EXPECT_TRUE(buffersEqual(inBuffer, outBuffer));

	ControlBuffer controlBuffer;

	Graph g;
	makeBasicGraph(g, numIo);
	g.init(numSamples, 44100);
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
	g.init(numSamples, 44100);

	for (int i = 0; i < 1000; ++i)
	{
		g.process(outBuffer, controlBuffer);
		ASSERT_TRUE(buffersEqual(inBuffer, outBuffer));
	}
}

TEST(Graph, TestSerialization)
{
	registerBuiltInModules();

	const size_t numIo = 5;
	const size_t bufferSize = 1024;
	const float testValue = 0.5f;

	Graph g;
	g.setNumAudioInputs(numIo);
	g.setNumAudioOutputs(numIo);

	auto* aIn = g.getInputModule();
	auto* aOut = g.getOutputModule();

	const size_t gmId = g.addModule(std::make_unique<GraphModule>());
	auto* gm = dynamic_cast<GraphModule*>(g.getModuleById(gmId));
	ASSERT_NE(gm, nullptr);
	gm->setNumAudioInputs(numIo);
	gm->setNumAudioOutputs(numIo);

	auto* gAIn = gm->getGraph().getInputModule();
	auto* gAOut = gm->getGraph().getOutputModule();

	for (size_t cIdx = 0; cIdx < numIo; ++cIdx)
	{
		EXPECT_TRUE(dc::Module::connectAudio(gAIn, cIdx, gAOut, cIdx));
		EXPECT_TRUE(dc::Module::connectAudio(aIn, cIdx, gm, cIdx));
		EXPECT_TRUE(dc::Module::connectAudio(gm, cIdx, aOut, cIdx));
	}

	using json = nlohmann::json;
	auto j1 = g.toJson();

	Graph g2;
	g2.fromJson(j1);

	auto j2 = g2.toJson();

	auto patch = json::diff(j1, j2);

	ASSERT_EQ(patch.size(), 0);
}

TEST(Graph, TestSerializationWithRemove)
{
	registerBuiltInModules();

	const size_t numIo = 5;
	const size_t bufferSize = 1024;
	const float testValue = 0.5f;

	Graph g;
	g.setNumAudioInputs(numIo);
	g.setNumAudioOutputs(numIo);

	auto* aIn = g.getInputModule();
	auto* aOut = g.getOutputModule();

	size_t gmId = g.addModule(std::make_unique<GraphModule>());
	auto* gm = dynamic_cast<GraphModule*>(g.getModuleById(gmId));
	ASSERT_NE(gm, nullptr);
	gm->setNumAudioInputs(numIo);
	gm->setNumAudioOutputs(numIo);

	auto* gAIn = gm->getGraph().getInputModule();
	auto* gAOut = gm->getGraph().getOutputModule();

	for (size_t cIdx = 0; cIdx < numIo; ++cIdx)
	{
		EXPECT_TRUE(dc::Module::connectAudio(gAIn, cIdx, gAOut, cIdx));
		EXPECT_TRUE(dc::Module::connectAudio(aIn, cIdx, gm, cIdx));
		EXPECT_TRUE(dc::Module::connectAudio(gm, cIdx, aOut, cIdx));
	}

	using json = nlohmann::json;
	auto j1 = g.toJson();

	g.removeModuleById(gmId);

	{
		AudioBuffer inBuffer(bufferSize, numIo);
		inBuffer.fill(testValue);
		AudioBuffer outBuffer;
		outBuffer.copyFrom(inBuffer, true);
		EXPECT_TRUE(buffersEqual(inBuffer, outBuffer));
		ControlBuffer controlBuffer;

		g.init(bufferSize, 44100);
		g.process(outBuffer, controlBuffer);
	}

	gmId = g.addModule(std::make_unique<GraphModule>(), gmId);
	gm = dynamic_cast<GraphModule*>(g.getModuleById(gmId));
	ASSERT_NE(gm, nullptr);
	gm->setNumAudioInputs(numIo);
	gm->setNumAudioOutputs(numIo);

	for (size_t cIdx = 0; cIdx < numIo; ++cIdx)
	{
		EXPECT_TRUE(dc::Module::connectAudio(gAIn, cIdx, gAOut, cIdx));
		EXPECT_TRUE(dc::Module::connectAudio(aIn, cIdx, gm, cIdx));
		EXPECT_TRUE(dc::Module::connectAudio(gm, cIdx, aOut, cIdx));
	}

	auto j2 = g.toJson();

	auto patch = json::diff(j1, j2);

	ASSERT_EQ(patch.size(), 0);
}
