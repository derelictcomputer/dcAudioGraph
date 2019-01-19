#include <algorithm>
#include "Module.h"

dc::Module::Module() :
maxBlockSize(DEFAULT_MAX_BLOCK_SIZE),
maxAudioChannels(DEFAULT_MAX_AUDIO_CHANNELS),
maxControlChannels(DEFAULT_MAX_CONTROL_CHANNELS)
{
	// reserve a large audio buffer so we don't reallocate in the audio thread
	_audioBuffer.resize(maxBlockSize, maxAudioChannels);
	_controlBuffer.setNumChannels(maxControlChannels);

	_audioInputs.reserve(maxAudioChannels);
	_audioOutputs.reserve(maxAudioChannels);
	_controlInputs.reserve(maxControlChannels);
	_controlOutputs.reserve(maxControlChannels);
	_params.reserve(16);
}

size_t dc::Module::getNumAudioIo(bool isInput) const
{
	return isInput ? _numAudioInputs : _numAudioOutputs;
}

void dc::Module::setNumAudioIo(size_t num, bool isInput)
{
	num = std::min(num, maxAudioChannels);

	while (num < getNumAudioIo(isInput))
	{
		removeAudioIo(getNumAudioIo(isInput) - 1, isInput);
	}
	while (num > getNumAudioIo(isInput))
	{
		addAudioIo(isInput);
	}
}

dc::Module::Io* dc::Module::getAudioIoAt(size_t index, bool isInput)
{
	if (index < getNumAudioIo(isInput))
	{
		return isInput ? &_audioInputs[index] : &_controlInputs[index];
	}
	return nullptr;
}

size_t dc::Module::getNumControlIo(bool isInput) const
{
	return isInput ? _numControlInputs : _numControlOutputs;
}

void dc::Module::setNumControlIo(size_t num, bool isInput)
{
	num = std::min(num, maxControlChannels);

	while (num < getNumControlIo(isInput))
	{
		removeControlIo(getNumControlIo(isInput) - 1, isInput);
	}
	while (num > getNumControlIo(isInput))
	{
		addControlIo(isInput, ControlMessage::All);
	}
}

dc::Module::ControlIo* dc::Module::getControlIoAt(size_t index, bool isInput)
{
	if (index < getNumControlIo(isInput))
	{
		return isInput ? &_controlInputs[index] : &_controlOutputs[index];
	}
	return nullptr;
}

dc::ModuleParam* dc::Module::getParamAt(size_t index)
{
	if (index < _params.size())
	{
		return &_params[index];
	}
	return nullptr;
}

dc::ModuleParam* dc::Module::getParamById(const std::string& id)
{
	for (auto& p : _params)
	{
		if (p.getId() == id)
		{
			return &p;
		}
	}
	return nullptr;
}

bool dc::Module::addAudioIo(bool isInput)
{
	if (getNumAudioIo(isInput) == maxAudioChannels)
	{
		return false;
	}

	if (isInput)
	{
		_audioInputs.emplace_back("audio");
		_numAudioInputs = _audioInputs.size();
	}
	else
	{
		_audioOutputs.emplace_back("audio");
		_numAudioOutputs = _audioOutputs.size();
	}

	return true;
}

void dc::Module::removeAudioIo(size_t index, bool isInput)
{
	if (isInput)
	{
		if (index < _audioInputs.size())
		{
			_audioInputs.erase(_audioInputs.begin() + index);
			_numAudioInputs = _audioInputs.size();
		}
	}
	else if (index < _audioOutputs.size())
	{
		{
			_audioOutputs.erase(_audioOutputs.begin() + index);
			_numAudioOutputs = _audioOutputs.size();
		}
	}
}

bool dc::Module::addControlIo(bool isInput, ControlMessage::Type typeFlags)
{
	if (getNumControlIo(isInput) == maxControlChannels)
	{
		return false;
	}

	if (isInput)
	{
		_controlInputs.emplace_back("control", typeFlags);
		_numControlInputs = _controlInputs.size();
	}
	else
	{
		_controlOutputs.emplace_back("control", typeFlags);
		_numControlOutputs = _controlOutputs.size();
	}

	return true;
}

void dc::Module::removeControlIo(size_t index, bool isInput)
{
	if (isInput)
	{
		if (index < _controlInputs.size())
		{
			_controlInputs.erase(_controlInputs.begin() + index);
			_numControlInputs = _controlInputs.size();
		}
	}
	else if (index < _controlOutputs.size())
	{
		{
			_controlOutputs.erase(_controlOutputs.begin() + index);
			_numControlOutputs = _controlOutputs.size();
		}
	}
}

void dc::Module::addParam(const std::string& id, const std::string& displayName, const ParamRange& range,
	bool serializable, bool hasControlInput)
{
	int controlInputIdx = -1;
	if (hasControlInput)
	{
		controlInputIdx = static_cast<int>(_controlInputs.size());
		addControlIo(true, ControlMessage::Float);
	}
	_params.emplace_back(id, displayName, range, serializable, controlInputIdx);
}


void dc::Module::updateBuffers()
{
	_audioBuffer.resize(_blockSize, std::max(_numAudioInputs, _numAudioOutputs));
	_controlBuffer.setNumChannels(std::max(_numControlInputs, _numControlOutputs));
}
