/*
  ==============================================================================

    ModuleBase.h
    Created: 25 Dec 2018 12:25:16pm
    Author:  Charlie Huguenard

  ==============================================================================
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
	friend class Graph;

	Module() = default;

	// for the love of Dog, don't copy/move these
	Module(const Module& other) = delete;
	Module& operator=(const Module& other) = delete;
	Module(Module&& other) = delete;
	Module& operator=(Module&& other) = delete;

	virtual ~Module() = default;

	virtual std::string getName() = 0;

	// Audio I/O
	size_t getNumAudioInputs() const { return _audioInputs.size(); }
	size_t getNumAudioOutputs() const { return _audioOutputs.size(); }

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

	// parameters
	size_t getNumParameters() const { return _params.size(); }
	ModuleParam* getParam(size_t index);
	ModuleParam* getParam(std::string id);

protected:
	void setBufferSize(size_t bufferSize);
	void setSampleRate(double sampleRate) { _sampleRate = sampleRate; }
	void setNumAudioInputs(size_t numInputs);
	void setNumAudioOutputs(size_t numOutputs);

	void addControlInput(std::string description, ControlMessage::Type typeFlags);
	void removeControlInput(size_t index);
	void addControlOutput(std::string description, ControlMessage::Type typeFlags);
	void removeControlOutput(size_t index);
	void addParam(const std::string& id, const std::string& displayName, ParamRange& range,
		bool serializable = false, bool hasControlInput = false);

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

	struct AudioInput : ModuleIo
	{
		AudioInput(Module& parent, size_t index) : ModuleIo(parent, index) {}

		std::vector<std::weak_ptr<AudioOutput>> connections;
	};

	struct ControlOutput : ModuleIo
	{
		ControlOutput(Module& parent, size_t index, ControlMessage::Type typeFlags) : ModuleIo(parent, index)
		{
			this->typeFlags = typeFlags;
		}

		ControlMessage::Type typeFlags;
	};

	struct ControlInput : ModuleIo
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

	// the id of this Module instance in its parent graph
	size_t _graphId = 0;
	// the last graph revision
	size_t _rev = 0;
};
}
