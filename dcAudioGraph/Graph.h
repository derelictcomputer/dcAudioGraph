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
class GraphOutputModule final : public Module
{
public:
	std::string getName() override { return "Graph Output"; }

protected:
	void onProcess() override {}
};

// a module for input into a graph
class GraphInputModule final : public Module
{
public:
	void setInputData(const AudioBuffer& inputBuffer, ControlBuffer& controlBuffer);

	std::string getName() override { return "Graph Input"; }

protected:
	void onProcess() override;
	void onRefreshAudioBuffers() override;
	void onRefreshControlBuffers() override;

private:
	AudioBuffer _inputAudioBuffer;
	ControlBuffer _inputControlBuffer;
};

class Graph
{
public:
	Graph();
	~Graph() = default;

	void init(size_t bufferSize, double sampleRate);
	void process(AudioBuffer& audioBuffer, ControlBuffer& controlBuffer);

	size_t getNumAudioInputs() const { return _inputModule.getNumAudioOutputs(); }
	void setNumAudioInputs(size_t numInputs);
	size_t getNumAudioOutputs() const { return _outputModule.getNumAudioOutputs(); }
	void setNumAudioOutputs(size_t numOutputs);

	size_t getNumControlInputs() const { return _inputModule.getNumControlOutputs(); }
	void setNumControlInputs(size_t numInputs);
	size_t getNumControlOutputs() const { return _outputModule.getNumControlOutputs(); }
	void setNumControlOutputs(size_t numOutputs);
	
	size_t addModule(std::unique_ptr<Module> module, size_t id = 0);
	size_t getNumModules() const { return _modules.size(); }

	Module* getInputModule() { return &_inputModule; }
	Module* getOutputModule() { return &_outputModule; }
	Module* getModuleAt(size_t index);
	Module* getModuleById(size_t id);

	void removeModuleAt(size_t index);
	void removeModuleById(size_t id);
	void clear();

private:
	// serializable
	GraphInputModule _inputModule;
	GraphOutputModule _outputModule;
	std::vector<std::unique_ptr<Module>> _modules;
	
	// internals
	size_t _nextId = 1;
	size_t _rev = 0;
	size_t _bufferSize = 0;
	double _sampleRate = 0;
};
}
