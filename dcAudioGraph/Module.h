#pragma once

#include <memory>
#include "AudioBuffer.h"
#include "EventBuffer.h"
#include "ModuleParam.h"

namespace dc
{
enum IoType : uint8_t
{
	Audio   = 0x01,
    Event   = 0x02,
	Input   = 0x10,
	Output  = 0x20
};

constexpr IoType operator|(const IoType lhs, const IoType rhs)
{
	return static_cast<IoType>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

constexpr IoType operator&(const IoType lhs, const IoType rhs)
{
	return static_cast<IoType>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}

constexpr bool ioTypeMatches(const IoType lhs, const IoType rhs)
{
	return (lhs & rhs) == rhs;
}

const size_t MODULE_MAX_MESSAGES = 32;
const size_t MODULE_DEFAULT_MAX_IO = 16;
const size_t MODULE_DEFAULT_MAX_PARAMS = 16;
const size_t MODULE_DEFAULT_MAX_BLOCK_SIZE = 2048;

class Module
{
public:
	struct Io
	{
		std::string description = "";
		EventMessage::Type eventTypeFlags = EventMessage::All;
	};

	friend class Graph;

	Module() = default;
	virtual ~Module() = default;

	Module(const Module&) = delete;
	Module& operator=(const Module&) = delete;
	Module(Module&&) = delete;
	Module& operator=(Module&&) = delete;

	// just some user data in case you need to tag modules
	std::string tag = "";

	size_t getId() const { return _id; }

	void setSampleRate(double sampleRate);
	double getSampleRate() const { return _sampleRate; }
	void setBlockSize(size_t blockSize);
	size_t getBlockSize() const { return _blockSize; }

	// I/O
	size_t getNumIo(IoType typeFlags) const;
	std::string getIoDescription(IoType typeFlags, size_t index);
	EventMessage::Type getEventIoFlags(size_t index, bool isInput);
	bool setNumIo(IoType typeFlags, size_t n);
	bool addIo(IoType typeFlags,
		const std::string& description = "",
		EventMessage::Type controlType = EventMessage::None);
	bool removeIo(IoType typeFlags, size_t index);

	// Params
	size_t getNumParams() const { return _params.size(); }
	std::string getParamId(size_t index);
	ModuleParam* getParam(size_t index);
	ModuleParam* getParam(const std::string& id);

protected:
    struct ModuleProcessContext
    {
		size_t numAudioIn;
		size_t numAudioOut;
		size_t numEventIn;
		size_t numEventOut;
		size_t blockSize;
		double sampleRate;
		AudioBuffer audioBuffer;
		EventBuffer eventBuffer;
		std::vector<ModuleParam*> params;
    };

    virtual void process(ModuleProcessContext& context);

    virtual void sampleRateChanged() {}
	virtual void blockSizeChanged() {}
    virtual void ioCountChanged(IoType type, size_t count);

    void setEventIoFilters(IoType type, size_t index, EventMessage::Type filters);

	// Params
	bool addParam(const std::string& id, const std::string& displayName,
		const ParamRange& range,
		bool serializable = false, bool hasControlInput = false, float initialValue = 0.0f);
	bool removeParam(size_t index);

    // for use in process() only
    static void updateParams(ModuleProcessContext& context);

private:
	// I/O
	virtual bool setNumIoInternal(std::vector<Io>& io, size_t n);
	virtual bool addIoInternal(std::vector<Io>& io, const std::string& description, EventMessage::Type controlType);
	virtual bool removeIoInternal(std::vector<Io>& io, size_t index);

    void updateProcessContext();

	Io* getIo(IoType typeFlags, size_t index);

	double _sampleRate = 0;
	size_t _blockSize = 0;
	std::vector<Io> _audioInputs;
	std::vector<Io> _audioOutputs;
	std::vector<Io> _eventInputs;
	std::vector<Io> _eventOutputs;
	std::vector<std::unique_ptr<ModuleParam>> _params;
	std::vector<std::unique_ptr<ModuleParam>> _paramsToRelease;
	std::shared_ptr<ModuleProcessContext> _processContext;

	// for the Graph
	size_t _id = 0;
};
}
