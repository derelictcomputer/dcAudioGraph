#include "Graph.h"
#include <algorithm>

bool dc::Connection::operator==(const Connection& other) const
{
	return fromId == other.fromId &&
		fromIdx == other.fromIdx &&
		toId == other.toId &&
		toIdx == other.toIdx &&
		type == other.type;
}

bool dc::GraphProcessor::pushGraphMessage(const GraphProcessorMessage& msg)
{
	return _graphMessageQueue.push(msg);
}

void dc::GraphProcessor::process(AudioBuffer& audioBuffer, ControlBuffer& controlBuffer)
{
	handleMessages();
	handleGraphMessages();

	_input->handleMessages();
	_output->handleMessages();
	for (auto p : _processors)
	{
		p->handleMessages();
	}

	_input->_audioBuffer.copyFrom(audioBuffer, false);
	_input->_controlBuffer.clear();
	_input->_controlBuffer.merge(controlBuffer);

	_output->_audioBuffer.zero();
	_output->_controlBuffer.clear();

	processModule(_output);

	audioBuffer.copyFrom(_output->_audioBuffer, false);
	controlBuffer.clear();
	controlBuffer.merge(_output->_controlBuffer);

    // let the parent know they can clear out removed modules
	_parent.setReadyToReleaseModules();
}

void dc::GraphProcessor::processModule(ModuleProcessor* proc)
{
	if (proc->_rev == _rev)
	{
		return;
	}
	proc->_rev = _rev;

	std::vector<Connection> connections;
	if (getInputConnectionsForModule(proc, connections))
	{
		// process upstream modules
		for (auto& c : connections)
		{
			if (auto* upstream = getModuleById(c.fromId))
			{
				processModule(upstream);
			}
		}

		// pull in control and audio
		proc->_audioBuffer.zero();
		proc->_controlBuffer.clear();

		for (auto& c : connections)
		{
			if (auto* upstream = getModuleById(c.fromId))
			{
				switch (c.type)
				{
				case Connection::Audio:
				{
					if (c.fromIdx < upstream->_audioBuffer.getNumChannels())
					{
						proc->_audioBuffer.addFrom(upstream->_audioBuffer, c.fromIdx, c.toIdx);
					}
					break;
				}
				case Connection::Control:
				{
					if (c.fromIdx < upstream->_controlBuffer.getNumChannels())
					{
						proc->_controlBuffer.merge(upstream->_controlBuffer, c.fromIdx, c.toIdx);
					}
					break;
				}
				default:
					break;
				}
			}
		}
	}

	proc->process();
}

dc::ModuleProcessor* dc::GraphProcessor::getModuleById(size_t id)
{
	if (_input->_id == id)
	{
		return _input;
	}

	if (_output->_id == id)
	{
		return _output;
	}

	for (auto proc : _processors)
	{
		if (proc->_id == id)
		{
			return proc;
		}
	}

	return nullptr;
}

bool dc::GraphProcessor::getInputConnectionsForModule(ModuleProcessor* proc, std::vector<Connection>& connections)
{
	for (auto& c : _connections)
	{
		if (c.toId == proc->_id)
		{
			connections.push_back(c);
		}
	}
	return !connections.empty();
}

void dc::GraphProcessor::handleGraphMessages()
{
	GraphProcessorMessage msg{};
	while (_graphMessageQueue.pop(msg))
	{
		switch (msg.type)
		{
		case GraphProcessorMessage::InputModule:
			_input = msg.moduleParam;
			break;
		case GraphProcessorMessage::OutputModule:
			_output = msg.moduleParam;
			break;
		case GraphProcessorMessage::AddModule: 
			_processors.push_back(msg.moduleParam);
			break;
		case GraphProcessorMessage::RemoveModule:
            for (auto it = _processors.begin(); it != _processors.end(); ++it)
            {
                if (*it == msg.moduleParam)
                {
					_processors.erase(it);
					break;
                }
            }
			break;
		case GraphProcessorMessage::AddConnection: 
			_connections.emplace_back(msg.connectionParam);
			break;
		case GraphProcessorMessage::RemoveConnection:
            for (auto it = _connections.begin(); it != _connections.end(); ++it)
            {
                if (*it == msg.connectionParam)
                {
					_connections.erase(it);
					break;
                }
            }
			break;
		default: ;
		}
	}
}

dc::Graph::Graph() :
	Module(std::make_unique<GraphProcessor>(*this)),
	_graphProcessor(dynamic_cast<GraphProcessor*>(_processor.get()))
{
	GraphProcessorMessage msg{};
	{
		msg.type = GraphProcessorMessage::InputModule;
		msg.moduleParam = _input._processor.get();
		_graphProcessor->pushGraphMessage(msg);

		_input.setId(_nextModuleId++);
	}
	{
		msg.type = GraphProcessorMessage::OutputModule;
		msg.moduleParam = _output._processor.get();
		_graphProcessor->pushGraphMessage(msg);

		_output.setId(_nextModuleId++);
	}
}

void dc::Graph::process(AudioBuffer& audioBuffer, ControlBuffer& controlBuffer) const
{
	_graphProcessor->incRev();
	_graphProcessor->process(audioBuffer, controlBuffer);
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
	module->setId(id);
	
	_modules.push_back(std::move(module));

	GraphProcessorMessage msg{};
	msg.type = GraphProcessorMessage::AddModule;
	msg.moduleParam = _modules[_modules.size() - 1]->_processor.get();
	_graphProcessor->pushGraphMessage(msg);

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
	if (id == _input.getId())
	{
		return &_input;
	}

	if (id == _output.getId())
	{
		return &_output;
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

	GraphProcessorMessage msg{};
	msg.type = GraphProcessorMessage::AddConnection;
	msg.connectionParam = connection;
	_graphProcessor->pushGraphMessage(msg);

	_allConnections.push_back(connection);

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
			GraphProcessorMessage msg{};
			msg.type = GraphProcessorMessage::RemoveConnection;
			msg.connectionParam = connection;
			_graphProcessor->pushGraphMessage(msg);

			_allConnections.erase(_allConnections.begin() + i);
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
	_input.setBlockSize(_blockSize);
	_output.setBlockSize(_blockSize);
	for (auto& m : _modules)
	{
		m->setBlockSize(_blockSize);
	}
}

bool dc::Graph::addIoInternal(std::vector<Io>& io, const std::string& description, ControlMessage::Type controlType)
{
	if (!Module::addIoInternal(io, description, controlType))
	{
		return false;
	}

	if (&io == &_audioInputs)
	{
		return _input.addIo(Audio | Output, description, controlType);
	}
	if (&io == &_audioOutputs)
	{
		return _output.addIo(Audio | Input, description, controlType);
	}
	if (&io == &_controlInputs)
	{
		return _input.addIo(Control | Output, description, controlType);
	}
	if (&io == &_controlOutputs)
	{
		return _output.addIo(Control | Input, description, controlType);
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
		return _input.removeIo(Audio | Output, index);
	}
	if (&io == &_audioOutputs)
	{
		return _output.removeIo(Audio | Input, index);
	}
	if (&io == &_controlInputs)
	{
		return _input.removeIo(Control | Output, index);
	}
	if (&io == &_controlOutputs)
	{
		return _output.removeIo(Control | Input, index);
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
	case Connection::Audio: 
	{
		if (connection.fromIdx >= from->getNumIo(Audio | Output) || connection.toIdx >= to->getNumIo(Audio | Input))
		{
			return false;
		}
		break;
	}
	case Connection::Control: 
	{
		const auto fromFlags = from->getControlIoFlags(connection.fromIdx, false);
		const auto toFlags = to->getControlIoFlags(connection.toIdx, true);
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
    // release any old removed modules
	releaseRemovedModules();

    if (index >= _modules.size())
    {
		return false;
    }

    // disconnect the module
	disconnectModule(_modules[index]->_id);

	// let the processor know this one went away
	GraphProcessorMessage msg{};
	msg.type = GraphProcessorMessage::RemoveModule;
	msg.moduleParam = _modules[index]->_processor.get();
	_graphProcessor->pushGraphMessage(msg);

    // stick the module into the release pool
	_moduleReleasePool.emplace_back(_modules[index].release());
	_modules.erase(_modules.begin() + index);
	_readyToRelease.clear();

	return true;
}

void dc::Graph::releaseRemovedModules()
{
    if (_readyToRelease.test_and_set())
    {
		_moduleReleasePool.clear();
		_readyToRelease.clear();
    }
}
