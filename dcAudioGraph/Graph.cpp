/*
  ==============================================================================

    Graph.cpp
    Created: 25 Dec 2018 12:23:17pm
    Author:  Charlie Huguenard

  ==============================================================================
*/

#include "Graph.h"
#include <string>

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
	_inputModule._graphId = _nextId++;
	_outputModule._graphId = _nextId++;
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
	while (numInputs < _inputModule.getNumControlInputs())
	{
		_inputModule.removeControlOutput(_inputModule.getNumControlInputs() - 1);
	}
	while (numInputs > _inputModule.getNumControlInputs())
	{
		const std::string description = "control " + std::to_string(_inputModule.getNumControlInputs());
		const auto typeFlags = ControlMessage::Type::All;
		_inputModule.addControlInput(description, typeFlags);
	}
}

void dc::Graph::setNumControlOutputs(size_t numOutputs)
{
	while (numOutputs < _outputModule.getNumControlInputs())
	{
		const size_t idx = _outputModule.getNumControlInputs() - 1;
		_outputModule.removeControlInput(idx);
		_outputModule.removeControlOutput(idx);
	}
	while (numOutputs > _outputModule.getNumControlInputs())
	{
		const std::string description = "control" + std::to_string(_outputModule.getNumControlInputs());
		const auto typeFlags = ControlMessage::Type::All;
		_outputModule.addControlInput(description, typeFlags);
		_outputModule.addControlOutput(description, typeFlags);
	}
}

size_t dc::Graph::addModule(std::unique_ptr<Module> module, size_t id)
{
	if (nullptr != module)
	{
		for (auto& m : _modules)
		{
			if (m->_graphId == id)
			{
				return 0;
			}
		}

		id = id > 0 ? id : _nextId++;
		module->_graphId = id;
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
	if (_inputModule._graphId == id)
	{
		return &_inputModule;
	}

	if (_outputModule._graphId == id)
	{
		return &_outputModule;
	}

	for (auto& m : _modules)
	{
		if (m->_graphId == id)
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
		if (_modules[i]->_graphId == id)
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

void dc::Graph::compressIds()
{
	_nextId = 1;
	_inputModule._graphId = _nextId++;
	_outputModule._graphId = _nextId++;
	for (auto& m : _modules)
	{
		m->_graphId = _nextId++;
	}
}
