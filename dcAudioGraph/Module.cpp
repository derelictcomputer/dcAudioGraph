#include <cassert>
#include "Module.h"

dc::Module::Module(std::unique_ptr<ModuleProcessor> processor) : _processor(std::move(processor))
{
	assert(nullptr != _processor);
}

void dc::Module::setSampleRate(double sampleRate)
{
	_sampleRate = sampleRate;
	ModuleProcessorMessage msg{};
	msg.type = ModuleProcessorMessage::SampleRate;
	msg.doubleParam = sampleRate;
	_processor->pushMessage(msg);
}

void dc::Module::setBlockSize(size_t blockSize)
{
	if (blockSize > MODULE_DEFAULT_MAX_BLOCK_SIZE)
	{
		return;
	}

	_blockSize = blockSize;

	ModuleProcessorMessage msg{};
	msg.type = ModuleProcessorMessage::BlockSize;
	msg.sizeParam = blockSize;
	_processor->pushMessage(msg);

	blockSizeChanged();
}

size_t dc::Module::getNumIo(IoType typeFlags) const
{
	size_t numIo = 0;

	if (typeFlags & Audio)
	{
		if (typeFlags & Input)
		{
			numIo += _audioInputs.size();
		}
		if (typeFlags & Output)
		{
			numIo += _audioOutputs.size();
		}
	}
	if (typeFlags & Control)
	{
		if (typeFlags & Input)
		{
			numIo += _controlInputs.size();
		}
		if (typeFlags & Output)
		{
			numIo += _controlOutputs.size();
		}
	}

	return numIo;
}

std::string dc::Module::getIoDescription(IoType typeFlags, size_t index)
{
	if (auto* io = getIo(typeFlags, index))
	{
		return io->description;
	}
	return "";
}

float dc::Module::getControlInputScale(size_t index)
{
	if (auto* io = getIo(Control | Input, index))
	{
		return io->scale;
	}
	return 0.0f;
}

void dc::Module::setControlInputScale(size_t index, float value)
{
	if (auto* io = getIo(Control | Input, index))
	{
		io->scale = value;
		ModuleProcessorMessage msg{};
		msg.type = ModuleProcessorMessage::ControlInputScaleChanged;
		msg.floatParam = value;
		_processor->pushMessage(msg);
	}
}

dc::ControlMessage::Type dc::Module::getControlIoFlags(size_t index, bool isInput)
{
	IoType ioType = Control;
	if (isInput)
	{
		ioType = ioType | Input;
	}
	else
	{
		ioType = ioType | Output;
	}

	if (auto* io = getIo(ioType, index))
	{
		return io->controlTypeFlags;
	}

	return ControlMessage::None;
}

bool dc::Module::setNumIo(IoType typeFlags, size_t n)
{
	bool success = true;

	if (typeFlags & Audio)
	{
		if (typeFlags & Input)
		{
			success &= setNumIoInternal(_audioInputs, n);
		}
		if (typeFlags & Output)
		{
			success &= setNumIoInternal(_audioOutputs, n);
		}
	}
	if (typeFlags & Control)
	{
		if (typeFlags & Input)
		{
			success &= setNumIoInternal(_controlInputs, n);
		}
		if (typeFlags & Output)
		{
			success &= setNumIoInternal(_controlOutputs, n);
		}
	}

	notifyIoChange(typeFlags);

	return success;
}

bool dc::Module::addIo(IoType typeFlags, const std::string& description, ControlMessage::Type controlType)
{
	bool success = true;

	if (typeFlags & Audio)
	{
		if (typeFlags & Input)
		{
			success &= addIoInternal(_audioInputs, description, controlType);
		}
		if (typeFlags & Output)
		{
			success &= addIoInternal(_audioOutputs, description, controlType);
		}
	}
	if (typeFlags & Control)
	{
		if (typeFlags & Input)
		{
			success &= addIoInternal(_controlInputs, description, controlType);
		}
		if (typeFlags & Output)
		{
			success &= addIoInternal(_controlOutputs, description, controlType);
		}
	}

	notifyIoChange(typeFlags);

	return success;
}

bool dc::Module::removeIo(IoType typeFlags, size_t index)
{
	bool success = true;

	if (typeFlags & Audio)
	{
		if (typeFlags & Input)
		{
			success &= removeIoInternal(_audioInputs, index);
		}
		if (typeFlags & Output)
		{
			success &= removeIoInternal(_audioOutputs, index);
		}
	}
	if (typeFlags & Control)
	{
		if (typeFlags & Input)
		{
			success &= removeIoInternal(_controlInputs, index);
		}
		if (typeFlags & Output)
		{
			success &= removeIoInternal(_controlOutputs, index);
		}
	}

	notifyIoChange(typeFlags);

	return success;
}

bool dc::Module::getParamRange(size_t index, ParamRange& rangeOut)
{
	if (auto* p = getParam(index))
	{
		rangeOut = p->getRange();
		return true;
	}
	return false;
}

bool dc::Module::getParamRange(const std::string& id, ParamRange& rangeOut)
{
	if (auto* p = getParam(id))
	{
		rangeOut = p->getRange();
		return true;
	}
	return false;
}

float dc::Module::getParamValue(size_t index)
{
	if (auto* p = getParam(index))
	{
		return p->getRaw();
	}
	return 0.0f;
}

float dc::Module::getParamValue(const std::string& id)
{
	if (auto* p = getParam(id))
	{
		return p->getRaw();
	}
	return 0.0f;
}

void dc::Module::setParamValue(size_t index, float value)
{
	if (auto* p = getParam(index))
	{
		p->setRaw(value);
		ModuleProcessorMessage msg{};
		msg.type = ModuleProcessorMessage::ParamChanged;
		msg.indexScalarParam = { index, p->getRaw() };
		_processor->pushMessage(msg);
	}
}

void dc::Module::setParamValue(const std::string& id, float value)
{
	for (size_t i = 0; i < _params.size(); ++i)
	{
		if (_params[i].getId() == id)
		{
			_params[i].setRaw(value);
			ModuleProcessorMessage msg{};
			msg.type = ModuleProcessorMessage::ParamChanged;
			msg.indexScalarParam = { i, _params[i].getRaw() };
			_processor->pushMessage(msg);
		}
	}
}

bool dc::Module::setNumIoInternal(std::vector<Io>& io, size_t n)
{
	while (n < io.size())
	{
		if (!removeIoInternal(io, io.size() - 1))
		{
			return false;
		}
	}
	while (n > io.size())
	{
		if (!addIoInternal(io, "", ControlMessage::All))
		{
			return false;
		}
	}
	return true;
}

bool dc::Module::addIoInternal(std::vector<Io>& io, const std::string& description, ControlMessage::Type controlType)
{
	if (io.size() < MODULE_DEFAULT_MAX_IO)
	{
		io.push_back({ description, controlType, 1.0f });
		return true;
	}
	return false;
}

bool dc::Module::removeIoInternal(std::vector<Io>& io, size_t index)
{
	if (index < io.size())
	{
		io.erase(io.begin() + index);
		return true;
	}
	return false;
}

bool dc::Module::addParam(const std::string& id, const std::string& displayName, const ParamRange& range,
	bool serializable, bool hasControlInput)
{
	if (_params.size() < MODULE_DEFAULT_MAX_PARAMS)
	{
		int inputIdx = -1;

		if (hasControlInput)
		{
			if (!addIo(Control | Input, displayName, ControlMessage::Float))
			{
				return false;
			}
			inputIdx = static_cast<int>(_controlInputs.size()) - 1;
		}

		_params.emplace_back(id, displayName, range, serializable, inputIdx);

		ModuleProcessorMessage msg{};
		msg.type = ModuleProcessorMessage::NumParams;
		msg.sizeParam = _params.size();
		_processor->pushMessage(msg);

		return true;
	}
	return false;
}

bool dc::Module::removeParam(size_t index)
{
	if (index < _params.size())
	{
		_params.erase(_params.begin() + index);

		ModuleProcessorMessage msg{};
		msg.type = ModuleProcessorMessage::NumParams;
		msg.sizeParam = _params.size();
		_processor->pushMessage(msg);

		return true;
	}
	return false;
}

void dc::Module::notifyIoChange(IoType typeFlags) const
{
	ModuleProcessorMessage msg{};
	if (typeFlags & Audio)
	{
		if (typeFlags & Input)
		{
			msg.type = ModuleProcessorMessage::NumAudioInputs;
			msg.sizeParam = _audioInputs.size();
			_processor->pushMessage(msg);
		}
		if (typeFlags & Output)
		{
			msg.type = ModuleProcessorMessage::NumAudioOutputs;
			msg.sizeParam = _audioOutputs.size();
			_processor->pushMessage(msg);
		}
	}
	if (typeFlags & Control)
	{
		if (typeFlags & Input)
		{
			msg.type = ModuleProcessorMessage::NumControlInputs;
			msg.sizeParam = _controlInputs.size();
			_processor->pushMessage(msg);
		}
		if (typeFlags & Output)
		{
			msg.type = ModuleProcessorMessage::NumControlOutputs;
			msg.sizeParam = _controlOutputs.size();
			_processor->pushMessage(msg);
		}
	}
}

dc::Module::Io* dc::Module::getIo(IoType typeFlags, size_t index)
{
	if (typeFlags & Audio)
	{
		if (typeFlags & Input)
		{
			if (index < _audioInputs.size())
			{
				return &_audioInputs[index];
			}
		}
		else if (typeFlags & Output)
		{
			if (index < _audioOutputs.size())
			{
				return &_audioOutputs[index];
			}
		}
	}
	else if (typeFlags & Control)
	{
		if (typeFlags & Input)
		{
			if (index < _controlInputs.size())
			{
				return &_controlInputs[index];
			}
		}
		else if (typeFlags & Output)
		{
			if (index < _controlOutputs.size())
			{
				return &_controlOutputs[index];
			}
		}
	}
	return nullptr;
}

dc::ModuleParam* dc::Module::getParam(size_t index)
{
	if (index < _params.size())
	{
		return &_params[index];
	}
	return nullptr;
}

dc::ModuleParam* dc::Module::getParam(const std::string& id)
{
	for (auto& param : _params)
	{
		if (param.getId() == id)
		{
			return &param;
		}
	}
	return nullptr;
}

void dc::Module::setId(size_t id)
{
	_id = id;
	ModuleProcessorMessage msg{};
	msg.type = ModuleProcessorMessage::Id;
	msg.sizeParam = id;
	_processor->pushMessage(msg);
}
