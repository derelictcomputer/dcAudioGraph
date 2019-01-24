#include "ModuleProcessor.h"
#include <algorithm>

dc::ModuleProcessor::ModuleProcessor() :
	_audioBuffer(MODULE_DEFAULT_MAX_BLOCK_SIZE, MODULE_DEFAULT_MAX_IO)
{
	_controlBuffer.setNumChannels(MODULE_DEFAULT_MAX_IO);
	_params.reserve(MODULE_DEFAULT_MAX_PARAMS);
}

void dc::ModuleProcessor::process()
{
	process(_audioBuffer, _controlBuffer);
}

bool dc::ModuleProcessor::pushMessage(const ModuleProcessorMessage& msg)
{
	return _messageQueue.push(msg);
}

bool dc::ModuleProcessor::pushMessage(const AddParamMessage& msg)
{
	return _addParamQueue.push(msg);
}

void dc::ModuleProcessor::handleMessages()
{
	{
		AddParamMessage msg;
		while (_addParamQueue.pop(msg))
		{
			_params.emplace_back("", "", msg.range, false, msg.controlInputIndex);
		}
	}
	{
		ModuleProcessorMessage msg{};
		while (_messageQueue.pop(msg))
		{
			switch (msg.type)
			{
			case ModuleProcessorMessage::Invalid:
				break;
			case ModuleProcessorMessage::Id:
				_id = msg.sizeParam;
				break;
			case ModuleProcessorMessage::SampleRate:
				_sampleRate = msg.doubleParam;
				sampleRateChanged();
				break;
			case ModuleProcessorMessage::BlockSize:
				_blockSize = msg.sizeParam;
				refreshAudioBuffer();
				blockSizeChanged();
				break;
			case ModuleProcessorMessage::NumAudioInputs:
				_numAudioInputs = msg.sizeParam;
				refreshAudioBuffer();
				audioIoChanged();
				break;
			case ModuleProcessorMessage::NumAudioOutputs:
				_numAudioOutputs = msg.sizeParam;
				refreshAudioBuffer();
				audioIoChanged();
				break;
			case ModuleProcessorMessage::NumControlInputs:
				_numControlInputs = msg.sizeParam;
				refreshControlBuffer();
				controlIoChanged();
				break;
			case ModuleProcessorMessage::NumControlOutputs:
				_numControlOutputs = msg.sizeParam;
				refreshControlBuffer();
				controlIoChanged();
				break;
			case ModuleProcessorMessage::RemoveParam:
				_params.erase(_params.begin() + msg.sizeParam);
				break;
			case ModuleProcessorMessage::ParamChanged:
			{
				const size_t idx = msg.indexScalarParam.index;
				if (idx < _params.size())
				{
					_params[idx].setRaw(msg.indexScalarParam.scalar);
					paramValueChanged(idx);
				}
				break;
			}
			default:;
			}
		}
	}
}

float dc::ModuleProcessor::getParamValue(size_t index)
{
	if (index < _params.size())
	{
		return _params[index].getRaw();
	}
	return 0.0f;
}

float dc::ModuleProcessor::getParamValueNormalized(size_t index)
{
    if (index < _params.size())
    {
		return _params[index].getNormalized();
    }
	return 0.0f;
}

float dc::ModuleProcessor::getRawValueFromNormalized(size_t paramIdx, float normalizedValue)
{
    if (paramIdx < _params.size())
    {
		return _params[paramIdx].getRange().getRaw(normalizedValue);
    }
	return 0.0f;
}

void dc::ModuleProcessor::refreshAudioBuffer()
{
	_audioBuffer.resize(_blockSize, std::max(_numAudioInputs, _numAudioOutputs));
}

void dc::ModuleProcessor::refreshControlBuffer()
{
	_controlBuffer.setNumChannels(std::max(_numControlInputs, _numControlOutputs));
}
