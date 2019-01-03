/*
  ==============================================================================

    ModuleBase.h
    Created: 25 Dec 2018 12:25:16pm
    Author:  Charlie Huguenard

  ==============================================================================
*/

#pragma once
#include <vector>
#include <memory>
#include "AudioBuffer.h"
#include "json.hpp"

namespace dc
{
using json = nlohmann::json;
class Graph;

class Module
{
public:
	Module() = default;
	virtual ~Module() = default;

	void setBufferSize(size_t bufferSize);
	void setSampleRate(double sampleRate) { _sampleRate = sampleRate; }
	void process(size_t rev);
	AudioBuffer& getOutputBuffer();

	size_t getNumAudioInputs() const { return _audioInputs.size(); }
	void setNumAudioInputs(size_t numInputs);
	size_t getNumAudioOutputs() const { return _audioOutputs.size(); }
	void setNumAudioOutputs(size_t numOutputs);

	static bool connectAudio(Module* from, size_t fromIdx, Module* to, size_t toIdx);

	json toJson() const;
	static std::unique_ptr<Module> createFromJson(const json& j);
	void fromJson(const json& j);
	void updateConnectionsFromJson(const json& j, Graph& parentGraph);

	size_t id = 0;

protected:
	virtual void onProcess() {}
	virtual void onRefreshBuffers() {}

	virtual nlohmann::json toJsonInternal() const = 0;
	virtual void fromJsonInternal(const nlohmann::json& j) = 0;

	virtual std::string getModuleIdForInstance() const = 0;

	AudioBuffer _buffer;
	double _sampleRate = 0;

private:
	struct AudioOutput
	{
		explicit AudioOutput(Module& parent, size_t index) : parent(parent), index(index) {}
		~AudioOutput() = default;

		Module& parent;
		size_t index;
	};

	struct AudioInput
	{
		explicit AudioInput(Module& parent) : parent(parent) {}
		~AudioInput() = default;

		Module& parent;
		std::vector<std::weak_ptr<AudioOutput>> outputs;
	};

	void refreshBuffers(size_t numSamples);

	std::vector<std::shared_ptr<AudioInput>> _audioInputs;
	std::vector<std::shared_ptr<AudioOutput>> _audioOutputs;
	size_t _rev = 0;
};
}
