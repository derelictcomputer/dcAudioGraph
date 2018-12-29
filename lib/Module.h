/*
  ==============================================================================

    ModuleBase.h
    Created: 25 Dec 2018 12:25:16pm
    Author:  charl

  ==============================================================================
*/

#pragma once
#include <vector>
#include <memory>
#include "AudioBuffer.h"

namespace dc
{
class Module
{
public:
	Module() = default;
	virtual ~Module() = default;

	void init(size_t bufferSize);
	void teardown();
	void process(size_t rev);
	AudioBuffer& getOutputBuffer();

	size_t getNumAudioInputs() const { return _audioInputs.size(); }
	void setNumAudioInputs(size_t numInputs);
	size_t getNumAudioOutputs() const { return _audioOutputs.size(); }
	void setNumAudioOutputs(size_t numOutputs);

	static bool connectAudio(Module* from, size_t fromIdx, Module* to, size_t toIdx);

	size_t id = 0;

protected:
	virtual void onInit(size_t /*bufferSize*/) {}
	virtual void onTeardown() {}
	virtual void onProcess() {}
	virtual void onRefreshIo(size_t /*bufferSize*/) {}

	AudioBuffer _processBuffer;

private:
	struct AudioOutput;

	struct AudioInput
	{
		explicit AudioInput(Module& parent) : parent(parent) {}
		~AudioInput() = default;

		Module& parent;
		std::vector<std::weak_ptr<AudioOutput>> outputs;
	};

	struct AudioOutput
	{
		explicit AudioOutput(Module& parent, size_t index) : parent(parent), index(index) {}
		~AudioOutput() = default;

		Module& parent;
		size_t index;
	};

	void refreshIo(size_t bufferSize);

	std::vector<std::shared_ptr<AudioInput>> _audioInputs;
	std::vector<std::shared_ptr<AudioOutput>> _audioOutputs;
	size_t _rev = 0;
};
}
