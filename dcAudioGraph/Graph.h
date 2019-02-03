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
	enum class Type
	{
		Audio,
		Control,
        Event
	};

	bool operator==(const Connection& other) const;
	bool operator!=(const Connection& other) const { return !(*this == other); }

	size_t fromId;
	size_t fromIdx;
	size_t toId;
	size_t toIdx;
	Type type;
};

class Graph final : public Module
{
public:
	Graph();

	void process(AudioBuffer& audio, AudioBuffer& control, EventBuffer& events) const;

	Module* getInputModule() { return &_inputModule; }
	Module* getOutputModule() { return &_outputModule; }

	void clear();

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

protected:
	void process(ModuleProcessContext& context) override;

private:
	// Just a passthrough for processing graph I/O
	// This also provides a way to connect modules in the graph to the outside world
	class GraphIoModule final : public Module
	{
	};

	void blockSizeChanged() override;
	bool addIoInternal(std::vector<Io>& io, const std::string& description, EventMessage::Type controlType) override;
	bool removeIoInternal(std::vector<Io>& io, size_t index) override;

	bool getModulesForConnection(const Connection& connection, Module*& from, Module*& to);
	bool connectionCreatesLoop(const Connection& connection);
	bool moduleIsInputTo(Module* from, Module* to);

	bool getInputConnectionsForModule(Module& m, std::vector<Connection>& connections);

	bool removeModuleInternal(size_t index);

	struct ModuleRenderInfo final
	{
		struct InputInfo
		{
			Module* module;
			Connection::Type type;
			size_t fromIdx;
			size_t toIdx;
		};

		Module* module = nullptr;
		std::vector<InputInfo> inputs;
	};

	struct GraphProcessContext final
	{
		std::vector<ModuleRenderInfo> modules;
	};

	static void processModule(ModuleRenderInfo& m);
	void updateGraphProcessContext();
	ModuleRenderInfo makeModuleRenderInfo(Module& m);

	GraphIoModule _inputModule;
	GraphIoModule _outputModule;
	std::vector<std::unique_ptr<Module>> _modules;
    std::vector<Connection> _allConnections;
	std::shared_ptr<GraphProcessContext> _graphProcessContext;
	std::vector<std::unique_ptr<Module>> _modulesToRelease;

	size_t _nextModuleId = 3; // reserve 0 for invalid, 1 and 2 for in and out
};
}
