/*
 * Usually the main entry point into the library.
 * A graph is a module that can contain other modules, 
 * and also provides an interface for an audio renderer to use.
 */

#pragma once

#include <memory>
#include "Module.h"

namespace dc
{
class Graph final : public Module
{
public:
	Graph();

	void setBlockSize(size_t blockSize);
	void setSampleRate(double sampleRate);

	void setNumAudioInputs(size_t count);
	void setNumAudioOutputs(size_t count);
	void setNumControlInputs(size_t count);
	void setNumControlOutputs(size_t count);

	void process(AudioBuffer& audioBuffer, ControlBuffer& controlBuffer, bool incrementRev = true);

	Module* getInputModule() { return &_input; }
	Module* getOutputModule() { return &_output; }

	size_t addModule(std::unique_ptr<Module> module);
	size_t getNumModules() const { return _modules.size(); }
	Module* getModuleAt(size_t index);
	Module* getModuleById(size_t id);
	void removeModuleAt(size_t index);
	void removeModuleById(size_t id);

	bool addConnection(const Connection& connection);
	void removeConnection(const Connection& connection);
	size_t getNumConnections() const { return _allConnections.size(); }
	bool getConnection(size_t index, Connection& connectionOut);
	void disconnectModule(size_t id);

protected:
	void process() override;
	void audioIoCountChanged() override;
	void controlIoCountChanged() override;

private:
	// Just a passthrough for processing graph I/O
	// This also provides a way to connect modules in the graph to the outside world
	class GraphIoModule final : public Module
	{
	protected:
		void process() override {}
	};

	bool connectionExists(const Connection& connection);
	bool getModulesForConnection(const Connection& connection, Module*& from, Module*& to);
	bool connectionCreatesLoop(const Connection& connection);
	bool moduleIsInputTo(Module* from, Module* to);

	GraphIoModule _input;
	GraphIoModule _output;
	std::vector<std::unique_ptr<Module>> _modules;
	std::vector<Connection> _allConnections;

	size_t _nextModuleId = 1;
};
}
