#include "Graph.h"
#include <algorithm>
#include <cassert>

bool dc::Connection::operator==(const Connection& other) const
{
	return fromId == other.fromId &&
		fromIdx == other.fromIdx &&
		toId == other.toId &&
		toIdx == other.toIdx &&
		type == other.type;
}

dc::Graph::Graph()
{
	_inputModule._id = _nextModuleId++;
	_outputModule._id = _nextModuleId++;
	updateGraphProcessContext();
}

void dc::Graph::process(ModuleProcessContext& context)
{
	process(context.audioBuffer, context.controlBuffer, context.eventBuffer);
}

void dc::Graph::process(AudioBuffer& audio, AudioBuffer& control, EventBuffer& events) const
{
	// get the context
	auto context = std::atomic_load(&_graphProcessContext);

	// this could be valid, so handle it
	if (nullptr == context || context->modules.empty())
	{
		return;
	}

	// copy input to input module
	if (auto* input = context->modules[0].module)
	{
		auto mCtx = std::atomic_load(&input->_processContext);
		mCtx->audioBuffer.copyFrom(audio, false);
		mCtx->controlBuffer.copyFrom(control, false);
		mCtx->eventBuffer.clear();
		mCtx->eventBuffer.merge(events);
	}
	else
	{
		assert(false);
		return;
	}

	// process the modules
	for (auto& m : context->modules)
	{
		processModule(m);
	}

	// copy output from output module
	if (auto* output = context->modules[context->modules.size() - 1].module)
	{
		auto mCtx = std::atomic_load(&output->_processContext);
		audio.copyFrom(mCtx->audioBuffer, false);
		control.copyFrom(mCtx->controlBuffer, false);
		events.clear();
		events.merge(mCtx->eventBuffer);
	}
	else
	{
		assert(false);
	}
}

void dc::Graph::processModule(ModuleRenderInfo& m)
{
	assert(nullptr != m.module);

	auto ctx = std::atomic_load(&m.module->_processContext);

    if (nullptr == ctx)
    {
		return;
    }

	// if this module has inputs, pull in the input data
	if (!m.inputs.empty())
	{
		ctx->audioBuffer.zero();
		ctx->controlBuffer.zero();
		ctx->eventBuffer.clear();

		for (auto inputInfo : m.inputs)
		{
			auto inCtx = std::atomic_load(&inputInfo.module->_processContext);

            if (nullptr == inCtx)
            {
                continue;
            }

			switch (inputInfo.type)
			{
			case Connection::Type::Audio:
				ctx->audioBuffer.addFrom(inCtx->audioBuffer, inputInfo.fromIdx, inputInfo.toIdx);
				break;
			case Connection::Type::Control:
				ctx->controlBuffer.addFrom(inCtx->controlBuffer, inputInfo.fromIdx, inputInfo.toIdx);
				break;
			case Connection::Type::Event:
				ctx->eventBuffer.merge(inCtx->eventBuffer, inputInfo.fromIdx, inputInfo.toIdx);
				break;
			default:;
			}
		}
	}

	m.module->process(*ctx);
}

void dc::Graph::updateGraphProcessContext()
{
	auto newContext = std::make_shared<GraphProcessContext>();

	// build the context
	newContext->modules.push_back(makeModuleRenderInfo(_inputModule));
	for (auto& m : _modules)
	{
		size_t insertIdx = 0;
		for (; insertIdx < newContext->modules.size(); ++insertIdx)
		{
			if (moduleIsInputTo(m.get(), newContext->modules[insertIdx].module))
			{
				break;
			}
		}
		auto renderInfo = makeModuleRenderInfo(*m);
		newContext->modules.insert(newContext->modules.begin() + insertIdx, renderInfo);
	}
	newContext->modules.push_back(makeModuleRenderInfo(_outputModule));

	// swap in the new context
	newContext = std::atomic_exchange(&_graphProcessContext, newContext);
	// spin here in case process() is still using the old context
	while (newContext.use_count() > 1) {}

	// now that the old context is gone, we can clear the released modules
	_modulesToRelease.clear();
}

dc::Graph::ModuleRenderInfo dc::Graph::makeModuleRenderInfo(Module& m)
{
	ModuleRenderInfo info;
	info.module = &m;

	std::vector<Connection> connections;
	if (getInputConnectionsForModule(m, connections))
	{
		for (auto& c : connections)
		{
			if (auto* upstream = getModuleById(c.fromId))
			{
				info.inputs.push_back({ upstream, c.type, c.fromIdx, c.toIdx });
			}
		}
	}

	return info;
}
size_t dc::Graph::addModule(std::unique_ptr<Module> module)
{
	if (nullptr == module)
	{
		return 0;
	}

	module->setBlockSize(_blockSize);
	module->setSampleRate(_sampleRate);

	const size_t id = _nextModuleId++;
	module->_id = id;
	
	_modules.push_back(std::move(module));

	updateGraphProcessContext();

	return id;
}

dc::Module* dc::Graph::getModuleAt(size_t index)
{
	if (index < _modules.size())
	{
		return _modules[index].get();
	}
	return nullptr;
}

dc::Module* dc::Graph::getModuleById(size_t id)
{
	if (id == _inputModule.getId())
	{
		return &_inputModule;
	}

	if (id == _outputModule.getId())
	{
		return &_outputModule;
	}

	for (auto& m : _modules)
	{
		if (m->getId() == id)
		{
			return m.get();
		}
	}
	return nullptr;
}

bool dc::Graph::removeModuleAt(size_t index)
{
	return removeModuleInternal(index);
}

bool dc::Graph::removeModuleById(size_t id)
{
	for (size_t i = 0; i < _modules.size(); ++i)
	{
		if (_modules[i]->getId() == id)
		{
			return removeModuleAt(i);
		}
	}
	return false;
}

bool dc::Graph::addConnection(const Connection& connection)
{
	if (!connectionIsValid(connection))
	{
		return false;
	}

	_allConnections.push_back(connection);

	updateGraphProcessContext();

	return true;
}

void dc::Graph::removeConnection(const Connection& connection)
{
	if (!connectionExists(connection))
	{
		return;
	}

	for (size_t i = 0; i < _allConnections.size(); ++i)
	{
		if (_allConnections[i] == connection)
		{
			_allConnections.erase(_allConnections.begin() + i);

			updateGraphProcessContext();

			break;
		}
	}
}

bool dc::Graph::getConnection(size_t index, Connection& connectionOut)
{
	if (index < _allConnections.size())
	{
		connectionOut = _allConnections[index];
		return true;
	}
	return false;
}

void dc::Graph::disconnectModule(size_t id)
{
	if (auto* m = getModuleById(id))
	{
		size_t i = 0;
		while (i < _allConnections.size())
		{
			auto& c = _allConnections[i];
			if (c.fromId == id || c.toId == id)
			{
				removeConnection(c);
			}
			else
			{
				++i;
			}
		}
	}
}

void dc::Graph::blockSizeChanged()
{
	_inputModule.setBlockSize(_blockSize);
	_outputModule.setBlockSize(_blockSize);
	for (auto& m : _modules)
	{
		m->setBlockSize(_blockSize);
	}
}

bool dc::Graph::addIoInternal(std::vector<Io>& io, const std::string& description, Event::Type controlType)
{
	if (!Module::addIoInternal(io, description, controlType))
	{
		return false;
	}

	if (&io == &_audioInputs)
	{
		return _inputModule.addIo(Audio | Output, description, controlType);
	}
	if (&io == &_audioOutputs)
	{
		return _outputModule.addIo(Audio | Input, description, controlType);
	}
	if (&io == &_controlInputs)
	{
		return _inputModule.addIo(Control | Output, description, controlType);
	}
	if (&io == &_controlOutputs)
	{
		return _outputModule.addIo(Control | Input, description, controlType);
	}
	if (&io == &_eventInputs)
	{
		return _inputModule.addIo(Event | Output, description, controlType);
	}
	if (&io == &_eventOutputs)
	{
		return _outputModule.addIo(Event | Input, description, controlType);
	}

	return false;
}

bool dc::Graph::removeIoInternal(std::vector<Io>& io, size_t index)
{
	if (!Module::removeIoInternal(io, index))
	{
		return false;
	}

	if (&io == &_audioInputs)
	{
		return _inputModule.removeIo(Audio | Output, index);
	}
	if (&io == &_audioOutputs)
	{
		return _outputModule.removeIo(Audio | Input, index);
	}
	if (&io == &_controlInputs)
	{
		return _inputModule.removeIo(Control | Output, index);
	}
	if (&io == &_controlOutputs)
	{
		return _outputModule.removeIo(Control | Input, index);
	}
	if (&io == &_eventInputs)
	{
		return _inputModule.removeIo(Event | Output, index);
	}
	if (&io == &_eventOutputs)
	{
		return _outputModule.removeIo(Event | Input, index);
	}

	return false;
}

bool dc::Graph::connectionIsValid(const Connection& connection)
{
	if (connectionExists(connection))
	{
		return false;
	}

	Module* from = nullptr;
	Module* to = nullptr;

	if (!getModulesForConnection(connection, from, to))
	{
		return false;
	}

	switch (connection.type)
	{
	case Connection::Type::Audio: 
	{
		if (connection.fromIdx >= from->getNumIo(Audio | Output) || connection.toIdx >= to->getNumIo(Audio | Input))
		{
			return false;
		}
		break;
	}
	case Connection::Type::Control: 
	{
		if (connection.fromIdx >= from->getNumIo(Control | Output) || connection.toIdx >= to->getNumIo(Control | Input))
		{
			return false;
		}
		break;
	}
	case Connection::Type::Event: 
	{
		const auto fromFlags = from->getEventIoFlags(connection.fromIdx, false);
		const auto toFlags = to->getEventIoFlags(connection.toIdx, true);
		if ((fromFlags & toFlags) == 0)
		{
			return false;
		}
		break;
	}
	default: 
		return false;
	}

	return !connectionCreatesLoop(connection);
}

bool dc::Graph::connectionExists(const Connection& connection)
{
	for (auto& c : _allConnections)
	{
		if (c == connection)
		{
			return true;
		}
	}
	return false;
}

bool dc::Graph::getModulesForConnection(const Connection& connection, Module*& from, Module*& to)
{
	from = getModuleById(connection.fromId);
	to = getModuleById(connection.toId);
	return nullptr != from && nullptr != to;
}

bool dc::Graph::connectionCreatesLoop(const Connection& connection)
{
	Module* from = nullptr;
	Module* to = nullptr;

	if (!getModulesForConnection(connection, from, to))
	{
		return false;
	}

	return moduleIsInputTo(to, from);
}

bool dc::Graph::moduleIsInputTo(Module* from, Module* to)
{
	if (from == to)
	{
		return true;
	}

	if (nullptr == from || nullptr == to)
	{
		return false;
	}

	std::vector<Connection> connections;
	if (getInputConnectionsForModule(*to, connections))
	{
		for (auto& c : connections)
		{
			if (auto* m = getModuleById(c.fromId))
			{
				if (moduleIsInputTo(from, m))
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool dc::Graph::getInputConnectionsForModule(Module& m, std::vector<Connection>& connections)
{
	for (auto& c : _allConnections)
	{
		if (c.toId == m.getId())
		{
			connections.push_back(c);
		}
	}
	return !connections.empty();
}

bool dc::Graph::removeModuleInternal(size_t index)
{
    if (index >= _modules.size())
    {
		return false;
    }

    // disconnect the module
	disconnectModule(_modules[index]->_id);

    // stick the module into the release pool
	_modulesToRelease.emplace_back(_modules[index].release());
	_modules.erase(_modules.begin() + index);

    // update the process context
	updateGraphProcessContext();

	return true;
}
