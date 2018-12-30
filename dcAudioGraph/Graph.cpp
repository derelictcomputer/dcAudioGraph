/*
  ==============================================================================

    Graph.cpp
    Created: 25 Dec 2018 12:23:17pm
    Author:  Charlie Huguenard

  ==============================================================================
*/

#include "Graph.h"

void dc::GraphInputModule::setInputData(const AudioBuffer& inputBuffer)
{
	for (size_t cIdx = 0; cIdx < _inputBuffer.getNumChannels(); ++cIdx)
	{
		if (cIdx < inputBuffer.getNumChannels())
		{
			_inputBuffer.copyFrom(inputBuffer, cIdx, cIdx);
		}
		else
		{
			_inputBuffer.zero(cIdx);
		}
	}
}

void dc::GraphInputModule::onProcess()
{
	for (size_t cIdx = 0; cIdx < getNumAudioOutputs(); ++cIdx)
	{
		_processBuffer.copyFrom(_inputBuffer, cIdx, cIdx);
	}
}

void dc::GraphInputModule::onRefreshIo(size_t bufferSize)
{
	_inputBuffer.resize(bufferSize, _processBuffer.getNumChannels());
}

dc::Graph::Graph()
{
	_inputModule.id = _nextId++;
	_outputModule.id = _nextId++;
}

void dc::Graph::init(size_t bufferSize)
{
	_bufferSize = bufferSize;
	_inputModule.init(bufferSize);
	_outputModule.init(bufferSize);
	for (auto& m : _modules)
	{
		m->init(bufferSize);
	}
}

void dc::Graph::teardown()
{
	_inputModule.teardown();
	_outputModule.teardown();
	for (auto& m : _modules)
	{
		m->teardown();
	}
}

void dc::Graph::process(const AudioBuffer& inputBuffer, AudioBuffer& outputBuffer)
{
	// copy the input buffer to the input module
	_inputModule.setInputData(inputBuffer);

	// process the graph
	_outputModule.process(++_rev);

	// copy the graph output to the output buffer
	auto& gOut = _outputModule.getOutputBuffer();
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
	_inputModule.setNumAudioOutputs(numInputs);
}

void dc::Graph::setNumAudioOutputs(size_t numOutputs)
{
	_outputModule.setNumAudioInputs(numOutputs);
	_outputModule.setNumAudioOutputs(numOutputs);
}

size_t dc::Graph::addModule(std::unique_ptr<Module> module)
{
	if (nullptr != module)
	{
		module->init(_bufferSize);
		const size_t id = _nextId++;
		module->id = id;
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
	for (auto& m : _modules)
	{
		if (m->id == id)
		{
			return m.get();
		}
	}

	return nullptr;
}
