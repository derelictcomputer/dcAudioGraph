/*
  ==============================================================================

    Graph.cpp
    Created: 25 Dec 2018 12:23:17pm
    Author:  Charlie Huguenard

  ==============================================================================
*/

#include "Graph.h"

using json = nlohmann::json;

void dc::GraphAudioInputModule::setInputData(const AudioBuffer& inputBuffer)
{
	_inputBuffer.zero();
	_inputBuffer.copyFrom(inputBuffer, false);
}

void dc::GraphAudioInputModule::onProcess()
{
	for (size_t cIdx = 0; cIdx < getNumAudioOutputs(); ++cIdx)
	{
		_buffer.copyFrom(_inputBuffer, cIdx, cIdx);
	}
}

void dc::GraphAudioInputModule::onRefreshBuffers()
{
	_inputBuffer.resize(_buffer.getNumSamples(), _buffer.getNumChannels());
}

dc::Graph::Graph()
{
	_audioInputModule.id = _nextId++;
	_audioOutputModule.id = _nextId++;
}

void dc::Graph::init(size_t bufferSize, double sampleRate)
{
	_bufferSize = bufferSize;
	_sampleRate = sampleRate;

	_audioInputModule.setBufferSize(bufferSize);
	_audioInputModule.setSampleRate(sampleRate);

	_audioOutputModule.setBufferSize(bufferSize);
	_audioOutputModule.setSampleRate(sampleRate);

	for (auto& m : _modules)
	{
		m->setBufferSize(bufferSize);
		m->setSampleRate(sampleRate);
	}
}

void dc::Graph::process(const AudioBuffer& inputBuffer, AudioBuffer& outputBuffer)
{
	// copy the input buffer to the input module
	_audioInputModule.setInputData(inputBuffer);

	// process the graph
	_audioOutputModule.process(++_rev);

	// copy the graph output to the output buffer
	auto& gOut = _audioOutputModule.getOutputBuffer();
	for (size_t cIdx = 0; cIdx < outputBuffer.getNumChannels(); ++cIdx)
	{
		if (cIdx < gOut.getNumChannels())
		{
			outputBuffer.copyFrom(gOut, cIdx, cIdx);
		}
		else
		{
			outputBuffer.zero(cIdx);
		}
	}
}

void dc::Graph::setNumAudioInputs(size_t numInputs)
{
	_audioInputModule.setNumAudioOutputs(numInputs);
}

void dc::Graph::setNumAudioOutputs(size_t numOutputs)
{
	_audioOutputModule.setNumAudioInputs(numOutputs);
	_audioOutputModule.setNumAudioOutputs(numOutputs);
}

size_t dc::Graph::addModule(std::unique_ptr<Module> module)
{
	if (nullptr != module)
	{
		const size_t id = _nextId++;
		module->id = id;
		module->setBufferSize(_bufferSize);
		module->setSampleRate(_sampleRate);
		_modules.push_back(std::move(module));
		return id;
	}
	return 0;
}

dc::Module* dc::Graph::getModuleAt(size_t index)
{
	if (index >= _modules.size())
	{
		return nullptr;
	}

	return _modules[index].get();
}

dc::Module* dc::Graph::getModuleById(size_t id)
{
	if (_audioInputModule.id == id)
	{
		return &_audioInputModule;
	}

	if (_audioOutputModule.id == id)
	{
		return &_audioOutputModule;
	}

	for (auto& m : _modules)
	{
		if (m->id == id)
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
		_modules.erase(_modules.begin() + index);
	}
}

void dc::Graph::removeModuleById(size_t id)
{
	for (size_t i = 0; i < _modules.size(); ++i)
	{
		if (_modules[i]->id == id)
		{
			_modules.erase(_modules.begin() + i);
			return;
		}
	}
}

void dc::Graph::clear()
{
	_modules.clear();
	// TODO: decide whether we should mess with the graph I/O
}

json dc::Graph::toJson() const
{
	json j;

	j["nextId"] = _nextId;
	j["audioInputModule"] = _audioInputModule.toJson();
	j["audioOutputModule"] = _audioOutputModule.toJson();
	j["modules"] = json::array();
	for (auto& module : _modules)
	{
		j["modules"].push_back(module->toJson());
	}

	return j;
}

void dc::Graph::fromJson(const json& j)
{
	clear();

	_audioInputModule.fromJson(j["audioInputModule"]);
	_audioOutputModule.fromJson(j["audioOutputModule"]);

	// first, create all the modules
	auto modules = j["modules"];
	for (auto& module : modules)
	{
		auto instance = Module::createFromJson(module);
		if (nullptr != instance)
		{
			addModule(std::move(instance));
		}
	}
	// now, connect them
	for (auto module : j["modules"])
	{
		const size_t graphId = module["_graphId"].get<size_t>();
		if (auto* instance = getModuleById(graphId))
		{
			instance->updateConnectionsFromJson(module, *this);
		}
	}
	// also connect the input and output modules
	_audioInputModule.updateConnectionsFromJson(j["audioInputModule"], *this);
	_audioOutputModule.updateConnectionsFromJson(j["audioOutputModule"], *this);

	_nextId = j["nextId"].get<size_t>();
}
