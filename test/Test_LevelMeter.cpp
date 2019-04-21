#include <thread>
#include "gtest/gtest.h"
#include "../dcAudioGraph/LevelMeter.h"
#include "../dcAudioGraph/Graph.h"

using namespace dc;

class TestLevelMeter : public ::testing::Test
{
public:
  void Init(size_t blockSize, size_t nChannels)
  {
    aBuf.resize(blockSize, nChannels);
    graph.setSampleRate(44100);
    graph.setBlockSize(blockSize);
    graph.setNumIo(Audio | Input | Output, nChannels);

    auto lmId = graph.addModule(std::make_unique<LevelMeter>());
    levelMeter = dynamic_cast<LevelMeter*>(graph.getModuleById(lmId));
    ASSERT_NE(levelMeter, nullptr);
    levelMeter->setNumIo(Audio | Input | Output, nChannels);
    EXPECT_EQ(levelMeter->getNumIo(Audio | Input), nChannels);
    EXPECT_EQ(levelMeter->getNumIo(Audio | Output), nChannels);

    auto* aIn = graph.getInputModule();
    auto* aOut = graph.getOutputModule();

    for (size_t cIdx = 0; cIdx < nChannels; ++cIdx)
    {
      graph.addConnection({aIn->getId(), cIdx, lmId, cIdx});
      graph.addConnection({lmId, cIdx, aOut->getId(), cIdx});
    }
  }

  void InitSawBuf()
  {
    const size_t nChannels = aBuf.getNumChannels();
    const size_t nSamples = aBuf.getNumSamples();

    // fill the buffer with a sawtooth
    for (size_t cIdx = 0; cIdx < nChannels; ++cIdx)
    {
      if (cIdx == 0)
      {
        auto* cPtr = aBuf.getChannelPointer(0);
        for (size_t sIdx = 0; sIdx < nSamples; ++sIdx)
        {
          const float val = ((float)sIdx / nSamples) * 2.0f - 1.0f;
          cPtr[sIdx] = val;
        }
      }
      else
      {
        aBuf.copyFrom(aBuf, 0, cIdx);
        // apply gain to each channel
        aBuf.applyGain(cIdx, (nChannels - cIdx) / (float)nChannels);
      }

      EXPECT_GT(aBuf.getPeak(cIdx), 0);
      EXPECT_GT(aBuf.getRms(cIdx), 0);
    }
  }

  Graph graph;
  LevelMeter* levelMeter;
  AudioBuffer aBuf;
  EventBuffer eBuf;
};

TEST_F(TestLevelMeter, Peak)
{
  const size_t nSamples = 2048;
  const size_t nChannels = 8;
  Init(nSamples, nChannels);
  InitSawBuf();

  const size_t numRuns = 16;

  for (size_t rIdx = 0; rIdx < numRuns; ++rIdx)
  {
    graph.process(aBuf, eBuf);

    for (size_t cIdx = 0; cIdx < nChannels; ++cIdx)
    {
      const float bufPeak = aBuf.getPeak(cIdx);
      const float lmPeak = levelMeter->getLevel(cIdx);
      EXPECT_FLOAT_EQ(lmPeak, bufPeak);
    }
  }
}

TEST_F(TestLevelMeter, Rms)
{
  const size_t nSamples = 512;
  const size_t nChannels = 5;
  Init(nSamples, nChannels);
  InitSawBuf();

  // set to RMS
  levelMeter->getParam(0)->setRaw(1);

  const size_t numRuns = 16;

  for (size_t rIdx = 0; rIdx < numRuns; ++rIdx)
  {
    graph.process(aBuf, eBuf);

    for (size_t cIdx = 0; cIdx < nChannels; ++cIdx)
    {
      const float bufRms = aBuf.getRms(cIdx);
      const float lmRms = levelMeter->getLevel(cIdx);
      EXPECT_FLOAT_EQ(lmRms, bufRms);
    }
  }
}

TEST_F(TestLevelMeter, Multithreaded)
{
  const size_t nSamples = 64;
  const size_t nChannels = 2;
  Init(nSamples, nChannels);
  InitSawBuf();

  const size_t nCtlUpdates = 23;
  const size_t nProcUpdates = 97;

  size_t ctlUpdateCount = 0;
  size_t procUpdateCount = 0;

  std::thread procThread([&]()
                         {
                           while (procUpdateCount < nProcUpdates)
                           {
                             graph.process(aBuf, eBuf);
                             std::this_thread::yield();
                             ++procUpdateCount;
                           }
                         });

  std::thread ctlThread([&]()
                        {
                          while (ctlUpdateCount < nCtlUpdates)
                          {
                            for (size_t cIdx = 0; cIdx < nChannels; ++cIdx)
                            {
                              const float bufLvl = aBuf.getPeak(cIdx);
                              const float lmLvl = levelMeter->getLevel(cIdx);
                              EXPECT_FLOAT_EQ(lmLvl, bufLvl) << "in update " << std::to_string(cIdx) << '\n';
                            }
                            std::this_thread::sleep_for(std::chrono::milliseconds(16));
                            ++ctlUpdateCount;
                          }
                        });

  ctlThread.join();
  procThread.join();

  EXPECT_EQ(nCtlUpdates, ctlUpdateCount);
  EXPECT_EQ(nProcUpdates, procUpdateCount);
}
