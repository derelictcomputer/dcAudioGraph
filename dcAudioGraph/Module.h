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

namespace dc
{
class Module
{
public:
	struct Connection
	{
		enum Type
		{
			Audio = 0,
			Control
		};

		bool operator==(const Connection& other) const;
		bool operator!=(const Connection& other) const { return !(*this == other); }

		size_t fromId;
		size_t fromIdx;
		size_t toId;
		size_t toIdx;
		Type type;
	};

	// let the parent graph reach in here
	friend class Graph;

	Module() = default;
	virtual ~Module() = default;

	// TODO: allow copy/move of modules, it'll be needed for eg. editor applications
	Module(const Module& other) = delete;
	Module& operator=(const Module& other) = delete;
	Module(Module&& other) = delete;
	Module& operator=(Module&& other) = delete;

	size_t getId() const { return _id; }

	size_t getBlockSize() const { return _blockSize; }
	double getSampleRate() const { return _sampleRate; }

	size_t getNumAudioInputs() const { return _audioInputs.size(); }
	size_t getNumAudioOutputs() const { return _audioOutputs.size(); }
	size_t getNumControlInputs() const { return _controlInputs.size(); }
	size_t getNumControlOutputs() const { return _controlOutputs.size(); }

protected:
	void addAudioIo(bool isInput);
	void removeAudioIo(size_t index, bool isInput);
	void addControlIo(bool isInput, ControlMessage::Type typeFlags);
	void removeControlIo(size_t index, bool isInput);

	// implement this to do something to audio or control
	virtual void process() = 0;

	// override these if you need to update your module for changes in size
	virtual void blockSizeChanged() {}
	virtual void sampleRateChanged() {}
	virtual void audioIoCountChanged() {}
	virtual void controlIoCountChanged() {}

	AudioBuffer _audioBuffer;
	ControlBuffer _controlBuffer;

private:
	struct Io
	{
		Io(std::string description) : description(std::move(description)) {}

		void removeConnection(const Connection& c);

		std::string description;
		std::vector<Connection> connections;
	};

	struct ControlIo : Io
	{
		ControlIo(std::string description, ControlMessage::Type typeFlags) : Io(description), typeFlags(typeFlags) {}

		ControlMessage::Type typeFlags;
	};

	void pullFromUpstream(Graph& parentGraph, size_t rev);

	void setBlockSizeInternal(size_t blockSize);
	void setSampleRateInternal(double sampleRate);
	void refreshAudioBuffer();
	void refreshControlBuffer();

	bool addConnectionInternal(const Connection& connection);
	void removeConnectionInternal(const Connection& connection);

	std::vector<Io> _audioInputs;
	std::vector<Io> _audioOutputs;
	std::vector<ControlIo> _controlInputs;
	std::vector<ControlIo> _controlOutputs;

	size_t _id = 0;
	size_t _rev = 0;
	size_t _blockSize = 0;
	double _sampleRate = 0;
};
}
