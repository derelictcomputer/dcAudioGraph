/*
  ==============================================================================

    Graph.h
    Created: 25 Dec 2018 12:23:17pm
    Author:  Charlie Huguenard

  ==============================================================================
*/

#pragma once
#include <vector>
#include "Module.h"

namespace dc
{
// a module for output from a graph
// Note: this module is really just a passthrough, but it helps for clarity
class GraphOutputModule : public Module
{
};

// a module for input into a graph
class GraphInputModule : public Module
{
public:
	void setInputData(const AudioBuffer& inputBuffer);

protected:
	void onProcess() final;
	void onRefreshBuffers() override;

private:
	AudioBuffer _inputBuffer;
};

class Graph
{
public:
	Graph();
	~Graph() = default;

	void init(size_t bufferSize, double sampleRate);
	void process(const AudioBuffer& inputBuffer, AudioBuffer& outputBuffer);

	size_t getNumAudioInputs() const { return _inputModule.getNumAudioOutputs(); }
	void setNumAudioInputs(size_t numInputs);
	size_t getNumAudioOutputs() const { return _outputModule.getNumAudioOutputs(); }
	void setNumAudioOutputs(size_t numOutputs);

	size_t addModule(std::unique_ptr<Module> module);
	Module* getAudioInput() { return &_inputModule; }
	Module* getAudioOutput() { return &_outputModule; }
	size_t getNumModules() const { return _modules.size(); }
	Module* getModuleAt(size_t index);
	Module* getModuleById(size_t id);

private:
	size_t _nextId = 1;
	size_t _rev = 0;
	std::vector<std::unique_ptr<Module>> _modules;
	GraphInputModule _inputModule;
	GraphOutputModule _outputModule;
	size_t _bufferSize = 0;
	double _sampleRate = 0;
};
}
