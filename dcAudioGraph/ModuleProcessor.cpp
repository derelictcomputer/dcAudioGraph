#include "ModuleProcessor.h"
#include <algorithm>

dc::ModuleProcessor::ModuleProcessor()
{
	_audioBuffer.resize(MODULE_DEFAULT_MAX_BLOCK_SIZE, MODULE_DEFAULT_MAX_IO);
	_controlBuffer.setNumChannels(MODULE_DEFAULT_MAX_IO);
	_paramValues.reserve(MODULE_DEFAULT_MAX_PARAMS);
	_controlInputScaleValues.reserve(MODULE_DEFAULT_MAX_IO);
}

void dc::ModuleProcessor::process()
{
	handleMessages();
	process(_audioBuffer, _controlBuffer);
}

bool dc::ModuleProcessor::pushMessage(const ModuleProcessorMessage& msg)
{
	return _messageQueue.push(msg);
}

float dc::ModuleProcessor::getParamValue(size_t index)
{
	if (index < _paramValues.size())
	{
		return _paramValues[index];
	}
	return 0.0f;
}

float dc::ModuleProcessor::getControlInputScale(size_t index)
{
	if (index < _controlInputScaleValues.size())
	{
		return _controlInputScaleValues[index];
	}
	return 0.0f;
}

void dc::ModuleProcessor::handleMessages()
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
			_controlInputScaleValues.resize(_numControlInputs);
			refreshControlBuffer();
			controlIoChanged();
			break;
		case ModuleProcessorMessage::NumControlOutputs:
			_numControlOutputs = msg.sizeParam;
			refreshControlBuffer();
			controlIoChanged();
			break;
		case ModuleProcessorMessage::NumParams:
			_paramValues.resize(msg.sizeParam);
			break;
		case ModuleProcessorMessage::ParamChanged:
			if (msg.indexScalarParam.index < _paramValues.size())
			{
				_paramValues[msg.indexScalarParam.index] = msg.indexScalarParam.scalar;
			}
			break;
		case ModuleProcessorMessage::ControlInputScaleChanged:
			if (msg.indexScalarParam.index < _controlInputScaleValues.size())
			{
				_controlInputScaleValues[msg.indexScalarParam.index] = msg.indexScalarParam.scalar;
			}
			break;
		default:;
		}
	}
}

void dc::ModuleProcessor::refreshAudioBuffer()
{
	_audioBuffer.resize(_blockSize, std::max(_numAudioInputs, _numAudioOutputs));
}

void dc::ModuleProcessor::refreshControlBuffer()
{
	_controlBuffer.setNumChannels(std::max(_numControlInputs, _numControlOutputs));
}
