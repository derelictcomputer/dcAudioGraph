#include "Graph.h"
#include <algorithm>

bool dc::Graph::Connection::operator==(const Connection& other) const
{
	return fromId == other.fromId &&
		fromIdx == other.fromIdx &&
		toId == other.toId &&
		toIdx == other.toIdx &&
		type == other.type;
}

dc::Graph::Graph()
{
	_input._id = _nextModuleId++;
	_output._id = _nextModuleId++;

	addParam("nCtlIn", "# Control Inputs", ParamRange(1, 16, 1), true, false);
	getParamAt(0)->setRaw(1);
	addParam("nCtlOut", "# Control Outputs", ParamRange(1, 16, 1), true, false);
	getParamAt(1)->setRaw(1);
	addParam("nAudIn", "# Audio Inputs", ParamRange(1, 16, 1), true, false);
	getParamAt(2)->setRaw(1);
	addParam("nAudOut", "# Audio Inputs", ParamRange(1, 16, 1), true, false);
	getParamAt(3)->setRaw(1);
}

void dc::Graph::setBlockSize(size_t blockSize)
{
	setBlockSizeInternal(blockSize);
	_input.setBlockSizeInternal(blockSize);
	_output.setBlockSizeInternal(blockSize);
	for (auto& m : _modules)
	{
		m->setBlockSizeInternal(blockSize);
	}
}

void dc::Graph::setSampleRate(double sampleRate)
{
	setSampleRateInternal(sampleRate);
	_input.setSampleRateInternal(sampleRate);
	_output.setSampleRateInternal(sampleRate);
	for (auto& m : _modules)
	{
		m->setSampleRateInternal(sampleRate);
	}
}

void dc::Graph::process(AudioBuffer& audioBuffer, ControlBuffer& controlBuffer, bool incrementRev)
{
	// update I/O
	{
		const size_t n = static_cast<size_t>(getParamAt(0)->getRaw());
		setNumControlIo(n, true);
	}
	{
		const size_t n = static_cast<size_t>(getParamAt(1)->getRaw());
		setNumControlIo(n, false);
	}
	{
		const size_t n = static_cast<size_t>(getParamAt(2)->getRaw());
		setNumAudioIo(n, true);
	}
	{
		const size_t n = static_cast<size_t>(getParamAt(3)->getRaw());
		setNumAudioIo(n, false);
	}

	// increment the graph revision (if this is the top level graph)
	if (incrementRev)
	{
		++_rev;
	}

	// copy the input to the input module
	{
		_input._audioBuffer.copyFrom(audioBuffer, false);
		_input._controlBuffer.clear();
		_input._controlBuffer.merge(controlBuffer);
	}

	// pull through audio and control
	{
		processModule(_output);
	}

	// copy the output from the output module
	{
		audioBuffer.copyFrom(_output._audioBuffer, false);
		controlBuffer.clear();
		controlBuffer.merge(_output._controlBuffer);
	}
}

size_t dc::Graph::addModule(std::unique_ptr<Module> module)
{
	if (nullptr == module)
	{
		return 0;
	}

	module->setBlockSizeInternal(_blockSize);
	module->setSampleRateInternal(_sampleRate);
	const size_t id = _nextModuleId++;
	module->_id = id;
	_modules.push_back(std::move(module));
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
		if (m->_id == id)
		{
			return m.get();
		}
	}
	return nullptr;
}

void dc::Graph::removeModuleAt(size_t index)
{
	if (index < _modules.size())
	{
		disconnectModule(_modules[index]->_id);
		_modules.erase(_modules.begin() + index);
	}
}

void dc::Graph::removeModuleById(size_t id)
{
	for (size_t i = 0; i < _modules.size(); ++i)
	{
		if (_modules[i]->_id == id)
		{
			removeModuleAt(i);
			return;
		}
	}
}

bool dc::Graph::addConnection(const Connection& connection)
{
	if (!connectionIsValid(connection))
	{
		return false;
	}

	_allConnections.push_back(connection);

	return true;
}

void dc::Graph::removeConnection(const Connection& connection)
{
	if (!connectionExists(connection))
	{
		return;
	}

	for (auto it = _allConnections.begin(); it != _allConnections.end(); ++it)
	{
		if (*it == connection)
		{
			_allConnections.erase(it);
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

void dc::Graph::process()
{
	process(_audioBuffer, _controlBuffer, false);
}

void dc::Graph::setNumAudioIo(size_t num, bool isInput)
{
	while (num < getNumAudioIo(isInput))
	{
		const size_t idx = getNumAudioIo(isInput) - 1;
		removeAudioIo(idx, isInput);
		if (isInput)
		{
			_input.removeAudioIo(idx, false);
		}
		else
		{
			_output.removeAudioIo(idx, true);
		}
	}
	while (num > getNumAudioIo(isInput))
	{
		addAudioIo(isInput);
		if (isInput)
		{
			_input.addAudioIo(false);
		}
		else
		{
			_output.addAudioIo(true);
		}
	}
}

void dc::Graph::setNumControlIo(size_t num, bool isInput)
{
	while (num < getNumControlIo(isInput))
	{
		const size_t idx = getNumControlIo(isInput) - 1;
		removeControlIo(idx, isInput);
		if (isInput)
		{
			_input.removeControlIo(idx, false);
		}
		else
		{
			_output.removeControlIo(idx, true);
		}
	}
	while (num > getNumControlIo(isInput))
	{
		addControlIo(isInput, ControlMessage::All);
		if (isInput)
		{
			_input.addControlIo(false, ControlMessage::All);
		}
		else
		{
			_output.addControlIo(true, ControlMessage::All);
		}
	}
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
		if (connection.fromIdx >= from->getNumAudioIo(false) || connection.toIdx >= to->getNumAudioIo(true))
		{
			return false;
		}
		break;
	}
	case Connection::Control: 
	{
		auto* fromOut = from->getControlIoAt(connection.fromIdx, false);
		auto* toIn = to->getControlIoAt(connection.toIdx, true);

		if (nullptr == fromOut || nullptr == toIn)
		{
			return false;
		}

		if ((fromOut->getTypeFlags() & toIn->getTypeFlags()) == 0)
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

void dc::Graph::processModule(Module& m)
{
	if (m._rev == _rev)
	{
		return;
	}
	m._rev = _rev;

	std::vector<Connection> connections;
	if (getInputConnectionsForModule(m, connections))
	{
		// process upstream modules
		for (auto& c : connections)
		{
			if (auto* upstream = getModuleById(c.fromId))
			{
				processModule(*upstream);
			}
		}

		// pull in control and audio
		m._audioBuffer.zero();
		m._controlBuffer.clear();

		for (auto& c : connections)
		{
			if (auto* upstream = getModuleById(c.fromId))
			{
				switch (c.type)
				{
				case Connection::Audio:
				{
					if (c.fromIdx < upstream->getNumAudioIo(false))
					{
						m._audioBuffer.addFrom(upstream->_audioBuffer, c.fromIdx, c.toIdx);
					}
					break;
				}
				case Connection::Control: 
				{
					if (c.fromIdx < upstream->getNumControlIo(false))
					{
						m._controlBuffer.merge(upstream->_controlBuffer, c.fromIdx, c.toIdx);
					}
					break;
				}
				default:
					break;
				}
			}
		}
	}

	m.process();
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
