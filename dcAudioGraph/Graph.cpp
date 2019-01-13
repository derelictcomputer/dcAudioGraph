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

void dc::Graph::setNumAudioInputs(size_t count)
{
	while (count < getNumAudioInputs())
	{
		removeAudioIo(getNumAudioInputs() - 1, true);
	}
	while (count > getNumAudioInputs())
	{
		addAudioIo(true);
	}
}

void dc::Graph::setNumAudioOutputs(size_t count)
{
	while (count < getNumAudioOutputs())
	{
		removeAudioIo(getNumAudioOutputs() - 1, false);
	}
	while (count > getNumAudioOutputs())
	{
		addAudioIo(false);
	}
}

void dc::Graph::setNumControlInputs(size_t count)
{
	while (count < getNumControlInputs())
	{
		removeControlIo(getNumControlInputs() - 1, true);
	}
	while (count > getNumControlInputs())
	{
		addControlIo(true, ControlMessage::All);
	}
}

void dc::Graph::setNumControlOutputs(size_t count)
{
	while (count < getNumControlOutputs())
	{
		removeControlIo(getNumControlOutputs() - 1, false);
	}
	while (count > getNumControlOutputs())
	{
		addControlIo(false, ControlMessage::All);
	}

}

void dc::Graph::process(AudioBuffer& audioBuffer, ControlBuffer& controlBuffer, bool incrementRev)
{
	// copy the input to the input module
	{
		_input._audioBuffer.copyFrom(audioBuffer, false);
		_input._controlBuffer.clear();
		_input._controlBuffer.merge(controlBuffer);
	}

	// pull through audio and control
	{
		_output.pullFromUpstream(*this, incrementRev ? ++_rev : _rev);
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
	if (connectionExists(connection))
	{
		return false;
	}

	if (connectionCreatesLoop(connection))
	{
		return false;
	}

	Module* from = nullptr;
	Module* to = nullptr;

	if (!getModulesForConnection(connection, from, to))
	{
		return false;
	}

	if (from->addConnectionInternal(connection))
	{
		if (to->addConnectionInternal(connection))
		{
			_allConnections.push_back(connection);
			return true;
		}
		else
		{
			from->removeConnectionInternal(connection);
		}
	}

	return false;
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

	Module* from = nullptr;
	Module* to = nullptr;

	if (getModulesForConnection(connection, from, to))
	{
		from->removeConnectionInternal(connection);
		to->removeConnectionInternal(connection);
	}
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

void dc::Graph::audioIoCountChanged()
{
	while (getNumAudioInputs() < _input.getNumAudioOutputs())
	{
		_input.removeAudioIo(_input.getNumAudioOutputs() - 1, false);
	}
	while (getNumAudioInputs() > _input.getNumAudioOutputs())
	{
		_input.addAudioIo(false);
	}
	while (getNumAudioOutputs() < _output.getNumAudioInputs())
	{
		_output.removeAudioIo(_output.getNumAudioInputs() - 1, true);
	}
	while (getNumAudioOutputs() > _output.getNumAudioInputs())
	{
		_output.addAudioIo(true);
	}
}

void dc::Graph::controlIoCountChanged()
{
	while (getNumControlInputs() < _input.getNumControlOutputs())
	{
		_input.removeControlIo(_input.getNumControlOutputs() - 1, false);
	}
	while (getNumControlInputs() > _input.getNumControlOutputs())
	{
		_input.addControlIo(false, ControlMessage::All);
	}
	while (getNumControlOutputs() < _output.getNumControlInputs())
	{
		_output.removeControlIo(_output.getNumControlInputs() - 1, true);
	}
	while (getNumControlOutputs() > _output.getNumControlInputs())
	{
		_output.addControlIo(true, ControlMessage::All);
	}
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

	for (auto& in : to->_audioInputs)
	{
		for (auto& c : in.connections)
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
	for (auto& in : to->_controlInputs)
	{
		for (auto& c : in.connections)
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
