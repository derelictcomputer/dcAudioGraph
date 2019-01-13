#include <algorithm>
#include "Graph.h"
#include "Module.h"

void dc::Module::addAudioIo(bool isInput)
{
	if (isInput)
	{
		_audioInputs.emplace_back("audio");
	}
	else
	{
		_audioOutputs.emplace_back("audio");
	}
	refreshAudioBuffer();
	audioIoCountChanged();
}

void dc::Module::removeAudioIo(size_t index, bool isInput)
{
	if (isInput)
	{
		if (index < _audioInputs.size())
		{
			_audioInputs.erase(_audioInputs.begin() + index);
		}
	}
	else
	{
		if (index < _audioOutputs.size())
		{
			_audioOutputs.erase(_audioOutputs.begin() + index);
		}
	}
	refreshAudioBuffer();
	audioIoCountChanged();
}

void dc::Module::addControlIo(bool isInput, ControlMessage::Type typeFlags)
{
	if (isInput)
	{
		_controlInputs.emplace_back("control", typeFlags);
	}
	else
	{
		_controlOutputs.emplace_back("control", typeFlags);
	}
	refreshControlBuffer();
	controlIoCountChanged();
}

void dc::Module::removeControlIo(size_t index, bool isInput)
{
	if (isInput)
	{
		if (index < _controlInputs.size())
		{
			_controlInputs.erase(_controlInputs.begin() + index);
		}
	}
	else
	{
		if (index < _controlOutputs.size())
		{
			_controlOutputs.erase(_controlOutputs.begin() + index);
		}
	}
	refreshControlBuffer();
	controlIoCountChanged();
}

void dc::Module::pullFromUpstream(Graph& parentGraph, size_t rev)
{
	if (rev == _rev)
	{
		return;
	}
	_rev = rev;

	// process upstream modules
	if (!_controlInputs.empty())
	{
		for (auto& in : _controlInputs)
		{
			for (auto& c : in.connections)
			{
				if (auto* upstream = parentGraph.getModuleById(c.fromId))
				{
					upstream->pullFromUpstream(parentGraph, rev);
				}
			}
		}
	}

	if (!_audioInputs.empty())
	{
		for (auto& in : _audioInputs)
		{
			for (auto& c : in.connections)
			{
				if (auto* upstream = parentGraph.getModuleById(c.fromId))
				{
					upstream->pullFromUpstream(parentGraph, rev);
				}
			}
		}
	}

	// pull in control and audio
	if (!_controlInputs.empty())
	{
		_controlBuffer.clear();

		for (auto& in : _controlInputs)
		{
			for (auto& c : in.connections)
			{
				if (auto* upstream = parentGraph.getModuleById(c.fromId))
				{
					// just in case there's a dead connection
					if (c.fromIdx >= upstream->getNumControlOutputs())
					{
						continue;
					}
					_controlBuffer.merge(upstream->_controlBuffer, c.fromIdx, c.toIdx);
				}
			}
		}
	}

	if (!_audioInputs.empty())
	{
		_audioBuffer.zero();

		for (auto& in : _audioInputs)
		{
			for (auto& c : in.connections)
			{
				if (auto* upstream = parentGraph.getModuleById(c.fromId))
				{
					if (c.fromIdx >= upstream->getNumAudioOutputs())
					{
						continue;
					}
					_audioBuffer.addFrom(upstream->_audioBuffer, c.fromIdx, c.toIdx);
				}
			}
		}
	}

	// run this module's process
	process();
}

void dc::Module::setBlockSizeInternal(size_t blockSize)
{
	_blockSize = blockSize;
	refreshAudioBuffer();
	blockSizeChanged();
}

void dc::Module::setSampleRateInternal(double sampleRate)
{
	_sampleRate = sampleRate;
	sampleRateChanged();
}

void dc::Module::refreshAudioBuffer()
{
	const size_t numChannels = std::max(_audioInputs.size(), _audioOutputs.size());
	_audioBuffer.resize(_blockSize, numChannels);
}

void dc::Module::refreshControlBuffer()
{
	const size_t numChannels = std::max(_controlInputs.size(), _controlOutputs.size());
	_controlBuffer.setNumChannels(numChannels);
}

bool dc::Module::addConnectionInternal(const Connection& connection)
{
	const auto type = connection.type;
	const auto fromId = connection.fromId;
	const auto toId = connection.toId;

	if (fromId == _id || toId == _id)
	{
		const bool isInput = toId == _id;
		const auto idx = isInput ? connection.toIdx : connection.fromIdx;

		switch (type)
		{
		case Connection::Audio:
			if (isInput)
			{
				if (idx < _audioInputs.size())
				{
					_audioInputs[idx].connections.push_back(connection);
					return true;
				}
			}
			else
			{
				if (idx < _audioOutputs.size())
				{
					_audioOutputs[idx].connections.push_back(connection);
					return true;
				}
			}
			break;
		case Connection::Control:
			if (isInput)
			{
				if (idx < _controlInputs.size())
				{
					_controlInputs[idx].connections.push_back(connection);
					return true;
				}
			}
			else
			{
				if (idx < _controlOutputs.size())
				{
					_controlOutputs[idx].connections.push_back(connection);
					return true;
				}
			}
			break;
		default:
			break;
		}
	}

	return false;
}

void dc::Module::Io::removeConnection(const Connection& c)
{
	size_t i = 0;
	while (i < connections.size())
	{
		if (connections[i] == c)
		{
			connections.erase(connections.begin() + i);
		}
		else
		{
			++i;
		}
	}
}

void dc::Module::removeConnectionInternal(const Connection& connection)
{
	const auto type = connection.type;
	const auto fromId = connection.fromId;
	const auto toId = connection.toId;

	if (fromId == _id || toId == _id)
	{
		const bool isInput = toId == _id;
		const auto idx = isInput ? connection.toIdx : connection.fromIdx;

		switch (type)
		{
		case Connection::Audio:
			if (isInput)
			{
				if (idx < _audioInputs.size())
				{
					_audioInputs[idx].removeConnection(connection);
				}
			}
			else
			{
				if (idx < _audioOutputs.size())
				{
					_audioOutputs[idx].removeConnection(connection);
				}
			}
			break;
		case Connection::Control:
			if (isInput)
			{
				if (idx < _controlInputs.size())
				{
					_controlInputs[idx].removeConnection(connection);
				}
			}
			else
			{
				if (idx < _controlOutputs.size())
				{
					_controlOutputs[idx].removeConnection(connection);
				}
			}
			break;
		default: 
			break;
		}
	}
}
