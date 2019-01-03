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
protected:
	json toJsonInternal() const override { return nullptr; }
	void fromJsonInternal(const json& j) override {}
	std::string getModuleIdForInstance() const override { return getModuleId(); }

private:
	// register with the ModuleFactory so we can deserialize
	static std::unique_ptr<Module> createMethod() { return std::make_unique<GraphAudioOutputModule>(); }
	static std::string getModuleId() { return "dc.GraphAudioOutputModule"; }
	static bool _registered;
};

// a module for input into a graph
class GraphAudioInputModule : public Module
{
public:
	void setInputData(const AudioBuffer& inputBuffer);

protected:
	void onProcess() final;
	void onRefreshBuffers() override;

	json toJsonInternal() const override { return nullptr; }
	void fromJsonInternal(const json& j) override {}
	std::string getModuleIdForInstance() const override { return getModuleId(); }

private:
	AudioBuffer _inputBuffer;

	// register with the ModuleFactory so we can deserialize
	static std::unique_ptr<Module> createMethod() { return std::make_unique<GraphAudioInputModule>(); }
	static std::string getModuleId() { return "dc.GraphAudioInputModule"; }
	static bool _registered;
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
