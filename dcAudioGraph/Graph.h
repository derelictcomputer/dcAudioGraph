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
struct Connection final
{
	enum Type
	{
		Audio = 0,
		Control
	};

	bool operator==(const Connection& other) const;
	bool operator!=(const Connection& other) const { return !(*this == other); }

	size_t fromId;
	size_t fromIdx;
	size_t toId;
	size_t toIdx;
	Type type;
};

struct GraphProcessorMessage
{
	enum Type
	{
		InputModule,
		OutputModule,
		AddModule,
		RemoveModule,
		AddConnection,
		RemoveConnection
	};

	Type type;

	union
	{
		ModuleProcessor* moduleParam;
		Connection connectionParam;
		size_t sizeParam;
	};
};

class GraphProcessor : public ModuleProcessor
{
public:
	explicit GraphProcessor(Graph& parent) : _parent(parent) {}

	void incRev() { ++_rev; }
	bool pushGraphMessage(const GraphProcessorMessage& msg);
	void process(AudioBuffer& audioBuffer, ControlBuffer& controlBuffer) override;

private:
	void handleGraphMessages();
	void processModule(ModuleProcessor* proc);
	ModuleProcessor* getModuleById(size_t id);
	bool getInputConnectionsForModule(ModuleProcessor* proc, std::vector<Connection>& connections);

	Graph& _parent;
	MessageQueue<GraphProcessorMessage> _graphMessageQueue{ MODULE_MAX_MESSAGES };
	ModuleProcessor* _input = nullptr;
	ModuleProcessor* _output = nullptr;
	std::vector<ModuleProcessor*> _processors;
	std::vector<Connection> _connections;
};

class Graph final : public Module
{
public:
	Graph();

	void process(AudioBuffer& audioBuffer, ControlBuffer& controlBuffer) const;

	Module* getInputModule() { return &_input; }
	Module* getOutputModule() { return &_output; }

	size_t addModule(std::unique_ptr<Module> module);
	size_t getNumModules() const { return _modules.size(); }
	Module* getModuleAt(size_t index);
	Module* getModuleById(size_t id);
	bool removeModuleAt(size_t index);
	bool removeModuleById(size_t id);

	bool connectionIsValid(const Connection& connection);
	bool connectionExists(const Connection& connection);
	bool addConnection(const Connection& connection);
	void removeConnection(const Connection& connection);
	size_t getNumConnections() const { return _allConnections.size(); }
	bool getConnection(size_t index, Connection& connectionOut);
	void disconnectModule(size_t id);

	void setReadyToReleaseModules() { _readyToRelease.test_and_set(); }

private:
	// Just a passthrough for processing graph I/O
	// This also provides a way to connect modules in the graph to the outside world
	class GraphIoModule final : public Module
	{
	public:
		class DummyProcessor : public ModuleProcessor
		{
			void process(AudioBuffer& /*audioBuffer*/, ControlBuffer& /*controlBuffer*/) override {}
		};

		GraphIoModule() : Module(std::make_unique<DummyProcessor>()) {}
	};

	void blockSizeChanged() override;
	bool addIoInternal(std::vector<Io>& io, const std::string& description, ControlMessage::Type controlType) override;
	bool removeIoInternal(std::vector<Io>& io, size_t index) override;

	bool getModulesForConnection(const Connection& connection, Module*& from, Module*& to);
	bool connectionCreatesLoop(const Connection& connection);
	bool moduleIsInputTo(Module* from, Module* to);

	bool getInputConnectionsForModule(Module& m, std::vector<Connection>& connections);

	bool removeModuleInternal(size_t index);
	void releaseRemovedModules();

	GraphProcessor* _graphProcessor;
	GraphIoModule _input;
	GraphIoModule _output;
	std::vector<std::unique_ptr<Module>> _modules;
    std::vector<Connection> _allConnections;

    // this is here to avoid deleting ModuleProcessors while the GraphProcessor is using them.
    // Order of operations for a "removeModule" call:
    //      1. Clear out the release pool if the flag is set
    //      2. Disconnect the module
    //      3. Tell the GraphProcessor the module went away
    //      4. Stick the module in the release pool
    //      5. Clear the flag
    //
    // Note: this means there's usually going to be a Module hanging around in the release pool.
	std::vector<std::unique_ptr<Module>> _moduleReleasePool;
	std::atomic_flag _readyToRelease = ATOMIC_FLAG_INIT;

	size_t _nextModuleId = 1;
};
}
