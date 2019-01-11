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

class Graph : public Module
{
public:
	Graph();

	std::string getName() override { return "Graph"; }

	void process(AudioBuffer& audioBuffer, ControlBuffer& controlBuffer);

	void setNumControlInputs(size_t numInputs);
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
	void onProcess() override;
	void onRefreshAudioBuffers() override;
	void onRefreshControlBuffers() override;

	// serializable
	GraphInputModule _inputModule;
	GraphOutputModule _outputModule;
	std::vector<std::unique_ptr<Module>> _modules;
	
	// internals
	size_t _nextId = 1;
};
}
