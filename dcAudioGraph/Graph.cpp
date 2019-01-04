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
		_audioBuffer.copyFrom(_inputBuffer, cIdx, cIdx);
	}
}

void dc::GraphAudioInputModule::onRefreshAudioBuffers()
{
	_inputBuffer.resize(_audioBuffer.getNumSamples(), _audioBuffer.getNumChannels());
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
	auto& gOut = _audioOutputModule.getAudioOutputBuffer();
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

size_t dc::Graph::addModule(std::unique_ptr<Module> module, size_t index)
{
	if (nullptr != module)
	{
		const size_t id = index > 0 ? index : _nextId++;
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

// serialization keys
const std::string S_AUDIO_INPUT_MODULE = "audioInputModule";
const std::string S_AUDIO_OUTPUT_MODULE = "audioOutputModule";
const std::string S_MODULES = "modules";

json dc::Graph::toJson() const
{
	json j;

	j[S_AUDIO_INPUT_MODULE] = _audioInputModule.toJson();
	j[S_AUDIO_OUTPUT_MODULE] = _audioOutputModule.toJson();
	
	auto m = json::array();
	for (auto& module : _modules)
	{
		m.push_back(module->toJson());
	}
	j[S_MODULES] = m;

	return j;
}

void dc::Graph::fromJson(const json& j)
{
	clear();

	_audioInputModule.fromJson(j[S_AUDIO_INPUT_MODULE]);
	_audioOutputModule.fromJson(j[S_AUDIO_OUTPUT_MODULE]);

	// first, create all the modules
	auto modules = j[S_MODULES];
	for (auto& module : modules)
	{
		auto instance = Module::createFromJson(module);
		if (nullptr != instance)
		{
			addModule(std::move(instance), instance->id);
		}
	}
	// now, connect them
	for (auto& module : modules)
	{
		Module::updateConnectionsFromJson(module, *this);
	}
	// also connect the input and output modules
	Module::updateConnectionsFromJson(j[S_AUDIO_INPUT_MODULE], *this);
	Module::updateConnectionsFromJson(j[S_AUDIO_OUTPUT_MODULE], *this);

	compressIds();
}

void dc::Graph::compressIds()
{
	_nextId = 1;
	_audioInputModule.id = _nextId++;
	_audioOutputModule.id = _nextId++;
	for (auto& m : _modules)
	{
		m->id = _nextId++;
	}
}
