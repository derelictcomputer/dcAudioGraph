/*
 * Usually the main entry point into the library.
 * A graph is a module that can contain other modules, 
 * and also provides an interface for an audio renderer to use.
 */

#pragma once

#include "Module.h"

namespace dc
{
// a module for output from a graph
// Note: this module is really just a passthrough
class GraphOutputModule final : public Module
{
protected:
	void onProcess() override {}
};

// a module for input into a graph
class GraphInputModule final : public Module
{
public:
	void setInputData(const AudioBuffer& inputBuffer, ControlBuffer& controlBuffer);

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
	Graph() = default;

	// Call this from your audio callback for the main graph to process all of your modules.
	void process(AudioBuffer& audioBuffer, ControlBuffer& controlBuffer, bool isTopLevel = true);

	void setNumControlInputs(size_t numInputs);
	void setNumControlOutputs(size_t numOutputs);

	size_t addModule(std::unique_ptr<Module> module);
	size_t getNumModules() const { return _modules.size(); }

	Module* getInputModule() { return &_inputModule; }
	Module* getOutputModule() { return &_outputModule; }
	Module* getModuleAt(size_t index);
	Module* getModuleById(size_t id);

	void removeModuleAt(size_t index);
	void removeModuleById(size_t id);
	void clear();

protected:
	void onProcess() override;
	void onRefreshAudioBuffers() override;
	void onRefreshControlBuffers() override;

private:
	// serializable
	GraphInputModule _inputModule;
	GraphOutputModule _outputModule;
	std::vector<std::unique_ptr<Module>> _modules;
};
}
