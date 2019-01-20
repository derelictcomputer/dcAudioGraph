#pragma once

#include <memory>
#include "ModuleParam.h"
#include "ModuleProcessor.h"

namespace dc
{
enum IoType : uint8_t
{
	Audio = 0x01,
	Control = 0x02,
	Input = 0x04,
	Output = 0x08
};

constexpr IoType operator|(const IoType lhs, const IoType rhs)
{
	return static_cast<IoType>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
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

	friend class Graph;

	explicit Module(std::unique_ptr<ModuleProcessor> processor);
	virtual ~Module() = default;

	Module(const Module&) = delete;
	Module& operator=(const Module&) = delete;
	Module(Module&&) = delete;
	Module& operator=(Module&&) = delete;

	size_t getId() const { return _id; }

	void setSampleRate(double sampleRate);
	double getSampleRate() const { return _sampleRate; }

	void setBlockSize(size_t blockSize);
	size_t getBlockSize() const { return _blockSize; }

	// I/O
	size_t getNumIo(IoType typeFlags) const;
	std::string getIoDescription(IoType typeFlags, size_t index);
	float getControlInputScale(size_t index);
	void setControlInputScale(size_t index, float value);
	ControlMessage::Type getControlIoFlags(size_t index, bool isInput);
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

	// for the Graph
	void setId(size_t id);
	size_t _id = 0;
};
}
