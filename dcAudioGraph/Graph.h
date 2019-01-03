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
#include "json.hpp"

namespace dc
{
// a module for output from a graph
// Note: this module is really just a passthrough, but it helps for clarity
class GraphAudioOutputModule : public Module
{
public:
	// convenience method for getting the unique id of the module type
	static std::string getModuleId() { return "dc.GraphAudioOutputModule"; }

protected:
	json toJsonInternal() const override { return nullptr; }
	void fromJsonInternal(const json& j) override {}
	std::string getModuleIdForInstance() const override { return getModuleId(); }
};

// a module for input into a graph
class GraphAudioInputModule : public Module
{
public:
	// convenience method for getting the unique id of the module type
	static std::string getModuleId() { return "dc.GraphAudioInputModule"; }

	void setInputData(const AudioBuffer& inputBuffer);

protected:
	void onProcess() final;
	void onRefreshBuffers() override;

	json toJsonInternal() const override { return nullptr; }
	void fromJsonInternal(const json& j) override {}
	std::string getModuleIdForInstance() const override { return getModuleId(); }

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

	size_t getNumAudioInputs() const { return _audioInputModule.getNumAudioOutputs(); }
	void setNumAudioInputs(size_t numInputs);
	Module* getAudioInput() { return &_audioInputModule; }
	size_t getNumAudioOutputs() const { return _audioOutputModule.getNumAudioOutputs(); }
	void setNumAudioOutputs(size_t numOutputs);
	Module* getAudioOutput() { return &_audioOutputModule; }

	size_t addModule(std::unique_ptr<Module> module);
	size_t getNumModules() const { return _modules.size(); }
	Module* getModuleAt(size_t index);
	Module* getModuleById(size_t id);
	void removeModuleAt(size_t index);
	void removeModuleById(size_t id);

	void clear();

	json toJson() const;
	void fromJson(const json& j);

private:
	// serializable
	size_t _nextId = 1;
	GraphAudioInputModule _audioInputModule;
	GraphAudioOutputModule _audioOutputModule;
	std::vector<std::unique_ptr<Module>> _modules;
	
	// internals
	size_t _rev = 0;
	size_t _bufferSize = 0;
	double _sampleRate = 0;
};
}
