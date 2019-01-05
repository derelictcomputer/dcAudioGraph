/*
  ==============================================================================

    Graph.cpp
    Created: 25 Dec 2018 12:23:17pm
    Author:  Charlie Huguenard

  ==============================================================================
*/

#include "Graph.h"

using json = nlohmann::json;

void dc::GraphInputModule::setInputData(const AudioBuffer& inputBuffer, ControlBuffer& controlBuffer)
{
	_inputAudioBuffer.zero();
	_inputAudioBuffer.copyFrom(inputBuffer, false);
	_inputControlBuffer.clear();
	_inputControlBuffer.merge(controlBuffer);
}

void dc::GraphInputModule::onProcess()
{
	for (size_t cIdx = 0; cIdx < getNumAudioOutputs(); ++cIdx)
	{
		_audioBuffer.copyFrom(_inputAudioBuffer, cIdx, cIdx);
	}
}

void dc::GraphInputModule::onRefreshAudioBuffers()
{
	_inputAudioBuffer.resize(_audioBuffer.getNumSamples(), _audioBuffer.getNumChannels());
}

void dc::GraphInputModule::onRefreshControlBuffers()
{
	_inputControlBuffer.setNumChannels(_controlBuffer.getNumChannels());
}

dc::Graph::Graph()
{
	_inputModule.id = _nextId++;
	_outputModule.id = _nextId++;
}

void dc::Graph::init(size_t bufferSize, double sampleRate)
{
	_bufferSize = bufferSize;
	_sampleRate = sampleRate;

	_inputModule.setBufferSize(bufferSize);
	_inputModule.setSampleRate(sampleRate);

	_outputModule.setBufferSize(bufferSize);
	_outputModule.setSampleRate(sampleRate);

	for (auto& m : _modules)
	{
		m->setBufferSize(bufferSize);
		m->setSampleRate(sampleRate);
	}
}

void dc::Graph::process(AudioBuffer& audioBuffer, ControlBuffer& controlBuffer)
{
	// copy the input buffers to the graph input
	_inputModule.setInputData(audioBuffer, controlBuffer);

	// process the graph
	_outputModule.process(++_rev);

	// copy the output
	audioBuffer.zero();
	audioBuffer.copyFrom(_outputModule.getAudioOutputBuffer(), false);
	controlBuffer.clear();
	controlBuffer.merge(_outputModule.getControlOutputBuffer());
}

void dc::Graph::setNumAudioInputs(size_t numInputs)
{
	_inputModule.setNumAudioOutputs(numInputs);
}

void dc::Graph::setNumAudioOutputs(size_t numOutputs)
{
	_outputModule.setNumAudioInputs(numOutputs);
	_outputModule.setNumAudioOutputs(numOutputs);
}

void dc::Graph::setNumControlInputs(size_t numInputs)
{
	_inputModule.setNumControlOutputs(numInputs);
}

void dc::Graph::setNumControlOutputs(size_t numOutputs)
{
	_outputModule.setNumControlInputs(numOutputs);
	_outputModule.setNumControlOutputs(numOutputs);
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
	if (_inputModule.id == id)
	{
		return &_inputModule;
	}

	if (_outputModule.id == id)
	{
		return &_outputModule;
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

	j[S_AUDIO_INPUT_MODULE] = _inputModule.toJson();
	j[S_AUDIO_OUTPUT_MODULE] = _outputModule.toJson();
	
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

	_inputModule.fromJson(j[S_AUDIO_INPUT_MODULE]);
	_outputModule.fromJson(j[S_AUDIO_OUTPUT_MODULE]);

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
	_inputModule.id = _nextId++;
	_outputModule.id = _nextId++;
	for (auto& m : _modules)
	{
		m->id = _nextId++;
	}
}
