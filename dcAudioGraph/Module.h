/*
 * This is where most of the work takes place.
 * A module has some number of audio and control I/O,
 * and pulls from anything that is connected to its inputs
 * before processing and passing to modules that are connected
 * to its outputs.
 * 
 * Derive from this to make concrete modules that actually do things
 * to sound and control.
 * 
 * Note: by default, a Module will pass through both audio and control.
 * If you don't want that, make sure to clear the buffers in your onProcess().
 * 
 */

#pragma once
#include <utility>
#include <vector>
#include "AudioBuffer.h"
#include "ControlBuffer.h"
#include "ModuleParam.h"
#include "PointerSwapper.h"

namespace dc
{
// defaults for buffer and I/O maximums
const size_t DEFAULT_MAX_BLOCK_SIZE = 1024;
const size_t DEFAULT_MAX_AUDIO_CHANNELS = 16;
const size_t DEFAULT_MAX_CONTROL_CHANNELS = 16;

class Module
{
public:
	class Io
	{
	public:
		explicit Io(std::string description) : _description(std::move(description)) {}

		std::string getDescription() const { return _description; }
		float getScale() const { return _scale; }
		void setScale(float scale) { _scale = scale; }

	private:
		std::string _description;
		float _scale = 1.0f;
	};

	class ControlIo final : public Io
	{
	public:
		ControlIo(std::string description, ControlMessage::Type typeFlags) : Io(description), _typeFlags(typeFlags) {}

		ControlMessage::Type getTypeFlags() const { return _typeFlags; }

	private:
		ControlMessage::Type _typeFlags;
	};

	// let the parent graph reach in here
	friend class Graph;

	Module();
	virtual ~Module() = default;

	// TODO: allow copy of modules, it'll be needed for eg. editor applications
	Module(const Module& other) = delete;
	Module& operator=(const Module& other) = delete;

	// disallow move
	Module(Module&& other) = delete;
	Module& operator=(Module&& other) = delete;

	size_t getId() const { return _id; }

	const size_t _maxBlockSize;
	const size_t _maxAudioChannels;
	const size_t _maxControlChannels;

	size_t getBlockSize() const { return _blockSize; }
	double getSampleRate() const { return _sampleRate; }

	size_t getNumAudioIo(bool isInput) const;
	virtual void setNumAudioIo(size_t num, bool isInput);
	Io* getAudioIoAt(size_t index, bool isInput);
	size_t getNumControlIo(bool isInput) const;
	virtual void setNumControlIo(size_t num, bool isInput);
	ControlIo* getControlIoAt(size_t index, bool isInput);

	size_t getNumParams() const { return _params.size(); }
	ModuleParam* getParamAt(size_t index);
	ModuleParam* getParamById(const std::string& id);

protected:
	void addAudioIo(bool isInput);
	void removeAudioIo(size_t index, bool isInput);
	void addControlIo(bool isInput, ControlMessage::Type typeFlags);
	void removeControlIo(size_t index, bool isInput);

	void addParam(const std::string& id, const std::string& displayName, const ParamRange& range, 
		bool serializable = false, bool hasControlInput = false);

	// implement this to do something to audio or control
	virtual void process() = 0;

	// only touch these in your process method
	AudioBuffer _audioBuffer;
	ControlBuffer _controlBuffer;

private:
	void updateBuffers();

	std::vector<Io> _audioInputs;
	std::vector<Io> _audioOutputs;
	std::vector<ControlIo> _controlInputs;
	std::vector<ControlIo> _controlOutputs;
	std::vector<ModuleParam> _params;

	size_t _id = 0;
	size_t _rev = 0;

	std::atomic<double> _sampleRate{ 0 };
	std::atomic<size_t> _blockSize{ 0 };
	std::atomic<size_t> _numAudioInputs{ 0 };
	std::atomic<size_t> _numAudioOutputs{ 0 };
	std::atomic<size_t> _numControlInputs{ 0 };
	std::atomic<size_t> _numControlOutputs{ 0 };
};
}
