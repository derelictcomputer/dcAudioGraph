#include "Module.h"
#include <algorithm>

void dc::Module::setSampleRate(double sampleRate)
{
	_sampleRate = sampleRate;
	updateProcessContext();
}

void dc::Module::setBlockSize(size_t blockSize)
{
	if (blockSize > MODULE_DEFAULT_MAX_BLOCK_SIZE)
	{
		return;
	}

	_blockSize = blockSize;
	updateProcessContext();
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
    if (typeFlags & Event)
    {
        if (typeFlags & Input)
        {
			numIo += _eventInputs.size();
        }
        if (typeFlags & Output)
        {
			numIo += _eventOutputs.size();
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

dc::EventMessage::Type dc::Module::getEventIoFlags(size_t index, bool isInput)
{
	IoType ioType = Event;
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
		return io->eventTypeFlags;
	}

	return EventMessage::None;
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
    if (typeFlags & Event)
	{
		if (typeFlags & Input)
		{
			success &= setNumIoInternal(_eventInputs, n);
		}
		if (typeFlags & Output)
		{
			success &= setNumIoInternal(_eventOutputs, n);
		}
	}

	if (success)
	{
		updateProcessContext();
		ioCountChanged(typeFlags, n);
	}

	return success;
}

bool dc::Module::addIo(IoType typeFlags, const std::string& description, EventMessage::Type controlType)
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
	if (typeFlags & Event)
	{
		if (typeFlags & Input)
		{
			success &= addIoInternal(_eventInputs, description, controlType);
		}
		if (typeFlags & Output)
		{
			success &= addIoInternal(_eventOutputs, description, controlType);
		}
	}

	if (success)
	{
		updateProcessContext();
		ioCountChanged(typeFlags, getNumIo(typeFlags));
	}

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
	if (typeFlags & Event)
	{
		if (typeFlags & Input)
		{
			success &= removeIoInternal(_eventInputs, index);
		}
		if (typeFlags & Output)
		{
			success &= removeIoInternal(_eventOutputs, index);
		}
	}

    if (success)
    {
		updateProcessContext();
		ioCountChanged(typeFlags, getNumIo(typeFlags));
    }

	return success;
}

std::string dc::Module::getParamId(size_t index)
{
	if (auto* p = getParam(index))
	{
		return p->getId();
	}
	return "";
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
		if (!addIoInternal(io, "", EventMessage::All))
		{
			return false;
		}
	}
	return true;
}

bool dc::Module::addIoInternal(std::vector<Io>& io, const std::string& description, EventMessage::Type controlType)
{
	if (io.size() < MODULE_DEFAULT_MAX_IO)
	{
		io.push_back({ description, controlType });
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

void dc::Module::setEventIoFilters(IoType type, size_t index, EventMessage::Type filters)
{
    if (type & Input)
    {
        if (index < _eventInputs.size())
        {
            _eventInputs[index].eventTypeFlags = filters;
        }
    }
    if (type & Output)
    {
        if (index < _eventOutputs.size())
        {
            _eventOutputs[index].eventTypeFlags = filters;
        }
    }
}

bool dc::Module::addParam(const std::string& id, const std::string& displayName, const ParamRange& range,
	bool serializable, bool hasControlInput, float initialValue)
{
	if (_params.size() < MODULE_DEFAULT_MAX_PARAMS)
	{
		int inputIdx = -1;

		if (hasControlInput)
		{
			if (!addIo(Event | Input, displayName, EventMessage::Type::Float))
			{
				return false;
			}
			inputIdx = static_cast<int>(_eventInputs.size()) - 1;
		}

		_params.emplace_back(std::make_unique<ModuleParam>(id, displayName, range, serializable, inputIdx, initialValue));

		updateProcessContext();

		return true;
	}
	return false;
}

bool dc::Module::removeParam(size_t index)
{
	if (index < _params.size())
	{
		// stick the param into the release pool
		_paramsToRelease.emplace_back(_params[index].release());
		_params.erase(_params.begin() + index);

        // update the context
		updateProcessContext();

		return true;
	}
	return false;
}

void dc::Module::updateParams(ModuleProcessContext& context)
{
    for (auto* param : context.params)
    {
        param->updateSmoothing(context.blockSize);

        if (param->hasControlInput())
        {
            auto it = context.eventBuffer.getIterator(param->getControlInputIndex());
            EventMessage msg;
            while (it.next(msg))
            {
                param->setControlInput(msg.floatParam.value);
            }
        }
    }
}

void dc::Module::updateProcessContext()
{
	auto newContext = std::make_shared<ModuleProcessContext>();

	newContext->numAudioIn = _audioInputs.size();
	newContext->numAudioOut = _audioOutputs.size();
	newContext->numEventIn = _eventInputs.size();
	newContext->numEventOut = _eventOutputs.size();
	newContext->blockSize = _blockSize;
	newContext->sampleRate = _sampleRate;
	newContext->audioBuffer.resize(_blockSize, std::max(_audioInputs.size(), _audioOutputs.size()));
	newContext->eventBuffer.setNumChannels(std::max(_eventInputs.size(), _eventOutputs.size()));
    for (auto& p : _params)
    {
		newContext->params.push_back(p.get());
    }

	// swap in the new context
	newContext = std::atomic_exchange(&_processContext, newContext);
	// spin here in case process() is still using the old context
	while (newContext.use_count() > 1) {}

	// now that the old context is gone, we can clear the released params
	_paramsToRelease.clear();
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
	else if (typeFlags & Event)
	{
		if (typeFlags & Input)
		{
			if (index < _eventInputs.size())
			{
				return &_eventInputs[index];
			}
		}
		else if (typeFlags & Output)
		{
			if (index < _eventOutputs.size())
			{
				return &_eventOutputs[index];
			}
		}
	}
	return nullptr;
}

dc::ModuleParam* dc::Module::getParam(size_t index)
{
	if (index < _params.size())
	{
		return _params[index].get();
	}
	return nullptr;
}

dc::ModuleParam* dc::Module::getParam(const std::string& id)
{
	for (auto& param : _params)
	{
		if (param->getId() == id)
		{
			return param.get();
		}
	}
	return nullptr;
}
