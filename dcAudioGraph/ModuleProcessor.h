#pragma once

#include "AudioBuffer.h"
#include "ControlBuffer.h"
#include "MessageQueue.h"
#include "ModuleParam.h"

namespace dc
{
const size_t MODULE_MAX_MESSAGES = 32;
const size_t MODULE_DEFAULT_MAX_IO = 16;
const size_t MODULE_DEFAULT_MAX_PARAMS = 16;
const size_t MODULE_DEFAULT_MAX_BLOCK_SIZE = 2048;

struct IndexScalarPair final
{
	size_t index;
	float scalar;
};

struct AddParamMessage
{
	ParamRange range;
	int controlInputIndex;
};

struct ModuleProcessorMessage final
{
	enum Type
	{
		Invalid = 0,
		Id,
		SampleRate,
		BlockSize,
		NumAudioInputs,
		NumAudioOutputs,
		NumControlInputs,
		NumControlOutputs,
        RemoveParam,
		ParamChanged
	};

	Type type;

	union
	{
		uint8_t noParam;
		size_t sizeParam;
		float floatParam;
		double doubleParam;
		IndexScalarPair indexScalarParam;
	};
};

class ModuleProcessor
{
public:
	friend class Graph;
	friend class GraphProcessor;

	ModuleProcessor();
	virtual ~ModuleProcessor() = default;

	ModuleProcessor(const ModuleProcessor&) = delete;
	ModuleProcessor& operator=(const ModuleProcessor&) = delete;
	ModuleProcessor(ModuleProcessor&& other) = delete;
	ModuleProcessor& operator=(ModuleProcessor&& other) = delete;

	// override this if your module should process even if it's not connected to an output
	virtual bool alwaysProcess() const { return false; }

	void process();
	bool pushMessage(const ModuleProcessorMessage& msg);
	bool pushMessage(const AddParamMessage& msg);
	void handleMessages();

protected:
	virtual void process(AudioBuffer& audioBuffer, ControlBuffer& controlBuffer) = 0;
	virtual void sampleRateChanged() {}
	virtual void blockSizeChanged() {}
	virtual void audioIoChanged() {}
	virtual void controlIoChanged() {}
    virtual void paramValueChanged(size_t /*paramIdx*/) {}

	double getSampleRate() const { return _sampleRate; }
	size_t getBlockSize() const { return _blockSize; }

	size_t getNumAudioInputs() const { return _numAudioInputs; }
	size_t getNumAudioOutputs() const { return _numAudioOutputs; }
	size_t getNumControlInputs() const { return _numControlInputs; }
	size_t getNumControlOutputs() const { return _numControlOutputs; }

	size_t getNumParams() const { return _params.size(); }
	float getParamValue(size_t index);
	float getParamValueNormalized(size_t index);
	float getRawValueFromNormalized(size_t paramIdx, float normalizedValue);

private:
	void refreshAudioBuffer();
	void refreshControlBuffer();

	MessageQueue<ModuleProcessorMessage> _messageQueue{ MODULE_MAX_MESSAGES };
	MessageQueue<AddParamMessage> _addParamQueue{ MODULE_MAX_MESSAGES };
	size_t _id = 0;
	size_t _rev = 0;
	double _sampleRate = 0;
	size_t _blockSize = 0;
	size_t _numAudioInputs = 0;
	size_t _numAudioOutputs = 0;
	size_t _numControlInputs = 0;
	size_t _numControlOutputs = 0;
	AudioBuffer _audioBuffer;
	ControlBuffer _controlBuffer;
	std::vector<ModuleParam> _params;
};
}
