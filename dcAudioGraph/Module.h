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
#include <memory>
#include <vector>
#include "AudioBuffer.h"
#include "ControlBuffer.h"
#include "ModuleParam.h"

namespace dc
{
class Module
{
public:
	// let graphs reach into here so they can do their work
	friend class Graph;

	Module();

	// for the love of Dog, don't copy/move these
	Module(const Module& other) = delete;
	Module& operator=(const Module& other) = delete;
	Module(Module&& other) = delete;
	Module& operator=(Module&& other) = delete;

	virtual ~Module() = default;

	// a unique id for this Module instance *DURING THIS RUN ONLY*
	// so, don't expect it to persist between sessions
	// this is mostly helpful for referring to a specific module in the graph at runtime
	size_t getId() const { return _id; }

	// Audio I/O
	void setBufferSize(size_t bufferSize);
	void setSampleRate(double sampleRate) { _sampleRate = sampleRate; }

	size_t getNumAudioInputs() const { return _audioInputs.size(); }
	size_t getNumAudioOutputs() const { return _audioOutputs.size(); }
	void setNumAudioInputs(size_t numInputs);
	void setNumAudioOutputs(size_t numOutputs);

	std::string getAudioInputDescription(size_t index) const;
	std::string getAudioOutputDescription(size_t index) const;

	static bool connectAudio(Module* from, Module* to);
	static bool connectAudio(Module* from, size_t fromIdx, Module* to, size_t toIdx);
	static void disconnectAudio(Module* from, Module* to);
	static void disconnectAudio(Module* from, size_t fromIdx, Module* to, size_t toIdx);
	void disconnectAudio();
	void removeDeadAudioConnections();

	// Control I/O
	size_t getNumControlInputs() const { return _controlInputs.size(); }
	size_t getNumControlOutputs() const { return _controlOutputs.size(); }

	std::string getControlInputDescription(size_t index) const;
	std::string getControlOutputDescription(size_t index) const;

	static bool connectControl(Module* from, size_t fromIdx, Module* to, size_t toIdx);
	static void disconnectControl(Module* from, Module* to);
	static void disconnectControl(Module* from, size_t fromIdx, Module* to, size_t toIdx);
	void disconnectControl();
	void removeDeadControlConnections();

	// Parameters
	size_t getNumParameters() const { return _params.size(); }
	ModuleParam* getParam(size_t index);
	ModuleParam* getParam(const std::string& id);

protected:
	// Control I/O
	void addControlInput(std::string description, ControlMessage::Type typeFlags);
	void removeControlInput(size_t index);
	void addControlOutput(std::string description, ControlMessage::Type typeFlags);
	void removeControlOutput(size_t index);

	// Parameters
	void addParam(const std::string& id, const std::string& displayName, const ParamRange& range,
		bool serializable = false, bool hasControlInput = false);

	// Processing
	void process(size_t rev);
	AudioBuffer& getAudioOutputBuffer() { return _audioBuffer; }
	ControlBuffer& getControlOutputBuffer() { return _controlBuffer; }
	void pushControlMessage(ControlMessage message, size_t outputIndex);

	// override these in your module to actually do things
	virtual void onProcess() = 0;
	virtual void onRefreshAudioBuffers() {}
	virtual void onRefreshControlBuffers() {}

	double _sampleRate = 0;
	AudioBuffer _audioBuffer;
	ControlBuffer _controlBuffer;

private:
	struct ModuleIo
	{
		ModuleIo(Module& parent, size_t index) : parent(parent), index(index) {}

		std::string description = "";
		Module& parent;
		size_t index;
	};

	using AudioOutput = ModuleIo;

	struct AudioInput final : ModuleIo
	{
		AudioInput(Module& parent, size_t index) : ModuleIo(parent, index) {}

		std::vector<std::weak_ptr<AudioOutput>> connections;
	};

	struct ControlOutput final : ModuleIo
	{
		ControlOutput(Module& parent, size_t index, ControlMessage::Type typeFlags) : ModuleIo(parent, index)
		{
			this->typeFlags = typeFlags;
		}

		ControlMessage::Type typeFlags;
	};

	struct ControlInput final : ModuleIo
	{
		ControlInput(Module& parent, size_t index, ControlMessage::Type typeFlags) : ModuleIo(parent, index)
		{
			this->typeFlags = typeFlags;
		}

		ControlMessage::Type typeFlags;
		std::vector<std::weak_ptr<ControlOutput>> connections;
	};

	void refreshAudioBuffers(size_t numSamples);
	void refreshControlBuffers();

	std::vector<std::unique_ptr<AudioInput>> _audioInputs;
	std::vector<std::shared_ptr<AudioOutput>> _audioOutputs;
	std::vector<std::unique_ptr<ControlInput>> _controlInputs;
	std::vector<std::shared_ptr<ControlOutput>> _controlOutputs;
	std::vector<std::unique_ptr<ModuleParam>> _params;

	// a unique id for this Module instance *DURING THIS RUN ONLY*
	// so, don't expect it to persist between sessions
	// this is mostly helpful for referring to a specific module in the graph at runtime
	size_t _id = 0;
	// the last graph revision
	size_t _rev = 0;
};
}
