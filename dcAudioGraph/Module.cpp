/*
  ==============================================================================

    ModuleBase.cpp
    Created: 25 Dec 2018 12:25:16pm
    Author:  Charlie Huguenard

  ==============================================================================
*/

#include <algorithm>
#include "Module.h"
#include <string>

std::string dc::Module::getAudioInputDescription(size_t index) const
{
	if (index < _audioInputs.size())
	{
		return _audioInputs[index]->description;
	}
	return "";
}

std::string dc::Module::getAudioOutputDescription(size_t index) const
{
	if (index < _audioOutputs.size())
	{
		return _audioOutputs[index]->description;
	}
	return "";
}

bool dc::Module::connectAudio(Module* from, Module* to)
{
	if (nullptr == from || nullptr == to)
	{
		return false;
	}

	const size_t numChannelsToConnect = std::min(from->getNumAudioOutputs(), to->getNumAudioInputs());

	for (size_t cIdx = 0; cIdx < numChannelsToConnect; ++cIdx)
	{
		if (!connectAudio(from, cIdx, to, cIdx))
		{
			return false;
		}
	}

	return true;
}

bool dc::Module::connectAudio(Module* from, size_t fromIdx, Module* to, size_t toIdx)
{
	if (nullptr == from || nullptr == to
		|| fromIdx >= from->getNumAudioOutputs() || toIdx >= to->getNumAudioInputs())
	{
		return false;
	}

	// check for duplicates
	auto& aIn = to->_audioInputs[toIdx];
	auto& targetAOut = from->_audioOutputs[fromIdx];
	for (auto& existingOut : aIn->connections)
	{
		if (auto eoPtr = existingOut.lock())
		{
			if (eoPtr.get() == targetAOut.get())
			{
				return false;
			}
		}
	}

	// TODO: check for loops, even though we technically handle those ok now

	aIn->connections.emplace_back(targetAOut);

	return true;
}

void dc::Module::disconnectAudio(Module* from, Module* to)
{
	if (nullptr == from || nullptr == to)
	{
		return;
	}

	for (auto& aIn : to->_audioInputs)
	{
		size_t aOutIdx = 0;
		while (aOutIdx < aIn->connections.size())
		{
			const auto connection = aIn->connections[aOutIdx].lock();

			// go ahead and delete dead connections
			if (!connection)
			{
				aIn->connections.erase(aIn->connections.begin() + aOutIdx);
				continue;
			}

			// if the connection parent is the from module, delete
			if (&connection->parent == from)
			{
				aIn->connections.erase(aIn->connections.begin() + aOutIdx);
				continue;
			}
			
			++aOutIdx;
		}
	}
}

void dc::Module::disconnectAudio(Module* from, size_t fromIdx, Module* to, size_t toIdx)
{
	if (nullptr == from || nullptr == to 
		|| fromIdx >= from->getNumAudioOutputs() || toIdx >= to->getNumAudioInputs())
	{
		return;
	}

	size_t aOutIdx = 0;
	auto& aIn = to->_audioInputs[toIdx];
	auto& aOut = from->_audioOutputs[fromIdx];
	while (aOutIdx < aIn->connections.size())
	{
		const auto existingOut = aIn->connections[aOutIdx].lock();
		if (existingOut && existingOut.get() == aOut.get())
		{
			aIn->connections.erase(aIn->connections.begin() + aOutIdx);
			continue;
		}
		++aOutIdx;
	}
}

void dc::Module::disconnectAudio()
{
	for (auto& aIn : _audioInputs)
	{
		aIn->connections.clear();
	}
}

void dc::Module::removeDeadAudioConnections()
{
	for (auto& aIn : _audioInputs)
	{
		for (size_t oIdx = 0; oIdx < aIn->connections.size();)
		{
			if (aIn->connections[oIdx].expired())
			{
				aIn->connections.erase(aIn->connections.begin() + oIdx);
				continue;
			}
			++oIdx;
		}
	}
}

std::string dc::Module::getControlInputDescription(size_t index) const
{
	if (index < _controlInputs.size())
	{
		return _controlInputs[index]->description;
	}
	return "";
}

std::string dc::Module::getControlOutputDescription(size_t index) const
{
	if (index < _controlOutputs.size())
	{
		return _controlOutputs[index]->description;
	}
	return "";
}

bool dc::Module::connectControl(Module* from, size_t fromIdx, Module* to, size_t toIdx)
{
	if (nullptr == from || nullptr == to
		|| fromIdx >= from->getNumControlOutputs() || toIdx >= to->getNumControlInputs())
	{
		return false;
	}

	auto& cIn = to->_controlInputs[toIdx];
	auto& targetCOut = from->_controlOutputs[fromIdx];

	// make sure the types match
	if ((cIn->typeFlags & targetCOut->typeFlags) == 0)
	{
		return false;
	}
	
	// check for duplicates
	for (auto& existingOut : cIn->connections)
	{
		if (auto eoPtr = existingOut.lock())
		{
			if (eoPtr.get() == targetCOut.get())
			{
				return false;
			}
		}
	}

	// TODO: check for loops, even though we technically handle those ok now

	cIn->connections.emplace_back(targetCOut);

	return false;
}

void dc::Module::disconnectControl(Module* from, Module* to)
{
	if (nullptr == from || nullptr == to)
	{
		return;
	}

	for (auto& cIn : to->_controlInputs)
	{
		size_t cOutIdx = 0;
		while (cOutIdx < cIn->connections.size())
		{
			const auto cOut = cIn->connections[cOutIdx].lock();

			// go ahead and delete dead connections
			if (!cOut)
			{
				cIn->connections.erase(cIn->connections.begin() + cOutIdx);
				continue;
			}

			if (&cOut->parent == from)
			{
				cIn->connections.erase(cIn->connections.begin() + cOutIdx);
				continue;
			}

			++cOutIdx;
		}
	}
}

void dc::Module::disconnectControl(Module* from, size_t fromIdx, Module* to, size_t toIdx)
{
	if (nullptr == from || nullptr == to
		|| fromIdx >= from->getNumControlOutputs() || toIdx >= to->getNumControlInputs())
	{
		return;
	}

	size_t cOutIdx = 0;
	auto& cIn = to->_controlInputs[toIdx];
	auto& cOut = from->_controlOutputs[fromIdx];
	while (cOutIdx < cIn->connections.size())
	{
		const auto existingOut = cIn->connections[cOutIdx].lock();
		if (existingOut && existingOut.get() == cOut.get())
		{
			cIn->connections.erase(cIn->connections.begin() + cOutIdx);
			continue;
		}
		++cOutIdx;
	}
}

void dc::Module::disconnectControl()
{
	for (auto& cIn : _controlInputs)
	{
		cIn->connections.clear();
	}
}

void dc::Module::removeDeadControlConnections()
{
	for (auto& cIn : _controlInputs)
	{
		for (size_t oIdx = 0; oIdx < cIn->connections.size();)
		{
			if (cIn->connections[oIdx].expired())
			{
				cIn->connections.erase(cIn->connections.begin() + oIdx);
				continue;
			}
			++oIdx;
		}
	}
}

dc::ModuleParam* dc::Module::getParam(size_t index)
{
	if (index < _params.size())
	{
		return _params[index].get();
	}
	return nullptr;
}

dc::ModuleParam* dc::Module::getParam(std::string id)
{
	for (size_t i = 0; i < _params.size(); ++i)
	{
		if (_params[i]->getId() == id)
		{
			return _params[i].get();
		}
	}
	return nullptr;
}

void dc::Module::setBufferSize(size_t bufferSize)
{
	refreshAudioBuffers(bufferSize);
}

void dc::Module::setNumAudioInputs(size_t numInputs)
{
	while (numInputs < _audioInputs.size())
	{
		_audioInputs.pop_back();
	}
	while (numInputs > _audioInputs.size())
	{
		const size_t idx = _audioInputs.size();
		_audioInputs.emplace_back(new AudioInput(*this, idx));
		_audioInputs[idx]->description = "audio " + std::to_string(idx);
	}
	refreshAudioBuffers(_audioBuffer.getNumSamples());
}

void dc::Module::setNumAudioOutputs(size_t numOutputs)
{
	while (numOutputs < _audioOutputs.size())
	{
		_audioOutputs.pop_back();
	}
	while (numOutputs > _audioOutputs.size())
	{
		const size_t idx = _audioOutputs.size();
		_audioOutputs.emplace_back(new AudioOutput(*this, idx));
		_audioOutputs[idx]->description = "audio " + std::to_string(idx);
	}
	refreshAudioBuffers(_audioBuffer.getNumSamples());
}

void dc::Module::addControlInput(std::string description, ControlMessage::Type typeFlags)
{
	const size_t idx = _controlInputs.size();
	_controlInputs.emplace_back(new ControlInput(*this, idx, typeFlags));
	_controlInputs[idx]->description = description;
	refreshControlBuffers();
}

void dc::Module::removeControlInput(size_t index)
{
	if (index < _controlInputs.size())
	{
		_controlInputs.erase(_controlInputs.begin() + index);
	}
}

void dc::Module::addControlOutput(std::string description, ControlMessage::Type typeFlags)
{
	const size_t idx = _controlOutputs.size();
	_controlOutputs.emplace_back(new ControlOutput(*this, idx, typeFlags));
	_controlOutputs[idx]->description = description;
	refreshControlBuffers();
}

void dc::Module::removeControlOutput(size_t index)
{
	if (index < _controlOutputs.size())
	{
		_controlOutputs.erase(_controlOutputs.begin() + index);
	}
}

void dc::Module::addParam(const std::string& id, const std::string& displayName, ParamRange& range, bool serializable,
	bool hasControlInput)
{
	int controlInputIdx = -1;
	if (hasControlInput)
	{
		controlInputIdx = static_cast<int>(_controlInputs.size());
		addControlInput(displayName, ControlMessage::Type::Float);
	}
	_params.emplace_back(new ModuleParam(id, displayName, range, serializable, controlInputIdx));
}

void dc::Module::process(size_t rev)
{
	// check revision, if already processed, return
	if (rev == _rev)
	{
		return;
	}
	_rev = rev;

	// process upstream modules
	{
		removeDeadAudioConnections();
		removeDeadControlConnections();

		for (auto& aIn : _audioInputs)
		{
			for (auto& connection : aIn->connections)
			{
				if (auto cPtr = connection.lock())
				{
					cPtr->parent.process(rev);
				}
			}
		}
		for (auto& cIn : _controlInputs)
		{
			for (auto& connection : cIn->connections)
			{
				if (auto cPtr = connection.lock())
				{
					cPtr->parent.process(rev);
				}
			}
		}
	}

	// pull in control data
	{
		_controlBuffer.clear();

		for (auto& cIn : _controlInputs)
		{
			for (auto& connection : cIn->connections)
			{
				if (auto cPtr = connection.lock())
				{
					_controlBuffer.merge(cPtr->parent.getControlOutputBuffer(), cPtr->index, cIn->index);
				}
			}
		}
	}

	// pull in audio data
	{
		_audioBuffer.zero();

		for (auto& aIn : _audioInputs)
		{
			for (auto& connection : aIn->connections)
			{
				if (auto cPtr = connection.lock())
				{
					_audioBuffer.addFrom(cPtr->parent.getAudioOutputBuffer(), cPtr->index, aIn->index);
				}
			}
		}
	}

	// process the control + audio
	onProcess();
}

void dc::Module::pushControlMessage(ControlMessage message, size_t outputIndex)
{
	_controlBuffer.insert(message, outputIndex);
}

void dc::Module::refreshAudioBuffers(size_t numSamples)
{
	_audioBuffer.resize(numSamples, std::max(_audioInputs.size(), _audioOutputs.size()));
	onRefreshAudioBuffers();
}

void dc::Module::refreshControlBuffers()
{
	_controlBuffer.setNumChannels(std::max(_controlInputs.size(), _controlOutputs.size()));
	onRefreshControlBuffers();
}
