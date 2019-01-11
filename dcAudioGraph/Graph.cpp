#include <string>
#include "Graph.h"

void dc::GraphInputModule::setInputData(const AudioBuffer& inputBuffer, ControlBuffer& controlBuffer)
{
	_inputAudioBuffer.zero();
	_inputAudioBuffer.copyFrom(inputBuffer, false);
	_inputControlBuffer.clear();
	_inputControlBuffer.merge(controlBuffer);
}

void dc::GraphInputModule::onProcess()
{
	_audioBuffer.copyFrom(_inputAudioBuffer, false);
	_controlBuffer.merge(_inputControlBuffer);
}

void dc::GraphInputModule::onRefreshAudioBuffers()
{
	_inputAudioBuffer.resize(_audioBuffer.getNumSamples(), _audioBuffer.getNumChannels());
}

void dc::GraphInputModule::onRefreshControlBuffers()
{
	_inputControlBuffer.setNumChannels(_controlBuffer.getNumChannels());
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

void dc::Graph::setNumControlInputs(size_t numInputs)
{
	while (numInputs < getNumControlInputs())
	{
		removeControlInput(getNumControlInputs() - 1);
	}
	while (numInputs > getNumControlInputs())
	{
		const std::string description = "control " + std::to_string(getNumControlInputs());
		const auto typeFlags = ControlMessage::Type::All;
		addControlInput(description, typeFlags);
	}
}

void dc::Graph::setNumControlOutputs(size_t numOutputs)
{
	while (numOutputs < getNumControlOutputs())
	{
		removeControlOutput(getNumControlOutputs() - 1);
	}
	while (numOutputs > getNumControlOutputs())
	{
		const std::string description = "control" + std::to_string(getNumControlOutputs());
		const auto typeFlags = ControlMessage::Type::All;
		addControlOutput(description, typeFlags);
	}
}

size_t dc::Graph::addModule(std::unique_ptr<Module> module)
{
	if (nullptr != module)
	{
		const size_t id = module->getId();

		// disallow adding a module twice to this graph
		// TODO: disallow a module being added in more than one place
		for (auto& m : _modules)
		{
			if (m->getId() == id)
			{
				return 0;
			}
		}

		module->setBufferSize(_audioBuffer.getNumChannels());
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
	if (_inputModule.getId() == id)
	{
		return &_inputModule;
	}

	if (_outputModule.getId() == id)
	{
		return &_outputModule;
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
		if (_modules[i]->getId() == id)
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

void dc::Graph::onProcess()
{
	process(_audioBuffer, _controlBuffer);
}

void dc::Graph::onRefreshAudioBuffers()
{
	const auto bufferSize = _audioBuffer.getNumSamples();
	const auto numChannels = _audioBuffer.getNumChannels();

	_inputModule.setBufferSize(bufferSize);
	_inputModule.setSampleRate(_sampleRate);
	_inputModule.setNumAudioOutputs(numChannels);

	_outputModule.setBufferSize(bufferSize);
	_outputModule.setSampleRate(_sampleRate);
	_outputModule.setNumAudioInputs(numChannels);
	_outputModule.setNumAudioOutputs(numChannels);

	for (auto& m : _modules)
	{
		m->setBufferSize(bufferSize);
		m->setSampleRate(_sampleRate);
	}
}

void dc::Graph::onRefreshControlBuffers()
{
	{
		const auto numChannels = getNumControlInputs();
		while (numChannels < _inputModule.getNumControlOutputs())
		{
			_inputModule.removeControlOutput(_inputModule.getNumControlOutputs() - 1);
		}
		while (numChannels > _inputModule.getNumControlOutputs())
		{
			const std::string description = "control " + std::to_string(_inputModule.getNumControlOutputs());
			const auto typeFlags = ControlMessage::Type::All;
			_inputModule.addControlOutput(description, typeFlags);
		}
	}
	{
		const auto numChannels = getNumControlInputs();
		while (numChannels < _outputModule.getNumControlOutputs())
		{
			_outputModule.removeControlInput(_outputModule.getNumControlInputs() - 1);
			_outputModule.removeControlOutput(_outputModule.getNumControlOutputs() - 1);
		}
		while (numChannels > _outputModule.getNumControlOutputs())
		{
			const std::string description = "control " + std::to_string(_outputModule.getNumControlOutputs());
			const auto typeFlags = ControlMessage::Type::All;
			_outputModule.addControlInput(description, typeFlags);
			_outputModule.addControlOutput(description, typeFlags);
		}
	}
}
