#pragma once

#include <memory>
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

struct ModuleProcessorMessage final
{
	enum Type
	{
		Invalid = 0,
		SampleRate,
		BlockSize,
		NumAudioInputs,
		NumAudioOutputs,
		NumControlInputs,
		NumControlOutputs,
		NumParams,
		ParamChanged,
		ControlInputScaleChanged
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
	ModuleProcessor();
	virtual ~ModuleProcessor() = default;

	ModuleProcessor(const ModuleProcessor&) = delete;
	ModuleProcessor& operator=(const ModuleProcessor&) = delete;
	ModuleProcessor(ModuleProcessor&& other) = delete;
	ModuleProcessor& operator=(ModuleProcessor&& other) = delete;

	void process();
	bool pushMessage(const ModuleProcessorMessage& msg);

protected:
	virtual void process(AudioBuffer& audioBuffer, ControlBuffer& controlBuffer) = 0;
	virtual void sampleRateChanged() {}
	virtual void blockSizeChanged() {}
	virtual void audioIoChanged() {}
	virtual void controlIoChanged() {}

	double getSampleRate() const { return _sampleRate; }
	size_t getBlockSize() const { return _blockSize; }

	size_t getNumAudioInputs() const { return _numAudioInputs; }
	size_t getNumAudioOutputs() const { return _numAudioOutputs; }
	size_t getNumControlInputs() const { return _numControlInputs; }
	size_t getNumControlOutputs() const { return _numControlOutputs; }

	size_t getNumParams() const { return _paramValues.size(); }
	float getParamValue(size_t index);

	float getControlInputScale(size_t index);

private:
	void handleMessages();

	MessageQueue<ModuleProcessorMessage> _messageQueue{ MODULE_MAX_MESSAGES };
	double _sampleRate = 0;
	size_t _blockSize = 0;
	size_t _numAudioInputs = 0;
	size_t _numAudioOutputs = 0;
	size_t _numControlInputs = 0;
	size_t _numControlOutputs = 0;
	AudioBuffer _audioBuffer;
	ControlBuffer _controlBuffer;
	std::vector<float> _paramValues;
	std::vector<float> _controlInputScaleValues;
};

enum IoType : uint8_t
{
	Audio = 0x01,
	Control = 0x02,
	Input = 0x04,
	Output = 0x08
};

constexpr IoType operator|(const IoType lhs, const IoType rhs)
{
	return static_cast<IoType>(lhs | rhs);
}

class Module
{
public:
	struct Io
	{
		std::string description = "";
		ControlMessage::Type controlTypeFlags = ControlMessage::All;
		float scale = 1.0f;
	};

	explicit Module(std::unique_ptr<ModuleProcessor> processor);
	virtual ~Module();

	Module(const Module&) = delete;
	Module& operator=(const Module&) = delete;
	Module(Module&&) = delete;
	Module& operator=(Module&&) = delete;

	void setSampleRate(double sampleRate);
	double getSampleRate() const { return _sampleRate; }

	void setBlockSize(size_t blockSize);
	size_t getBlockSize() const { return _blockSize; }

	// I/O
	size_t getNumIo(IoType typeFlags) const;
	std::string getIoDescription(IoType typeFlags, size_t index);
	float getControlInputScale(size_t index);
	void setControlInputScale(size_t index, float value);
	bool setNumIo(IoType typeFlags, size_t n);
	bool addIo(IoType typeFlags,
		const std::string& description = "",
		ControlMessage::Type controlType = ControlMessage::None);
	bool removeIo(IoType typeFlags, size_t index);

	// Params
	size_t getNumParams() const { return _params.size(); }
	bool getParamRange(size_t index, ParamRange& rangeOut);
	bool getParamRange(const std::string& id, ParamRange& rangeOut);
	float getParamValue(size_t index);
	float getParamValue(const std::string& id);
	void setParamValue(size_t index, float value);
	void setParamValue(const std::string& id, float value);

protected:
	// I/O
	virtual bool setNumIoInternal(std::vector<Io>& io, size_t n);
	virtual bool addIoInternal(std::vector<Io>& io, const std::string& description, ControlMessage::Type controlType);
	virtual bool removeIoInternal(std::vector<Io>& io, size_t index);

	// Params
	bool addParam(const std::string& id, const std::string& displayName,
		const ParamRange& range,
		bool serializable = false, bool hasControlInput = false);
	bool removeParam(size_t index);

private:
	void notifyIoChange(IoType typeFlags) const;
	Io* getIo(IoType typeFlags, size_t index);
	ModuleParam* getParam(size_t index);
	ModuleParam* getParam(const std::string& id);

	std::unique_ptr<ModuleProcessor> _processor;
	double _sampleRate = 0;
	size_t _blockSize = 0;
	std::vector<Io> _audioInputs;
	std::vector<Io> _audioOutputs;
	std::vector<Io> _controlInputs;
	std::vector<Io> _controlOutputs;
	std::vector<ModuleParam> _params;
};
}
