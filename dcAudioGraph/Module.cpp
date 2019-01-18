#include <algorithm>
#include "Module.h"

dc::Module::Module()
{
	_audioInputs.reserve(16);
	_audioOutputs.reserve(16);
	_controlInputs.reserve(16);
	_controlOutputs.reserve(16);
	_params.reserve(16);
}

size_t dc::Module::getNumAudioIo(bool isInput)
{
	return isInput ? _audioInputs.size() : _audioOutputs.size();
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
	return isInput ? _controlInputs.size() : _controlOutputs.size();
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
