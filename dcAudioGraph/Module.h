/*
  ==============================================================================

    ModuleBase.h
    Created: 25 Dec 2018 12:25:16pm
    Author:  Charlie Huguenard

  ==============================================================================
*/

#pragma once
#include <vector>
#include "AudioBuffer.h"
#include "ControlBuffer.h"
#include "json.hpp"

namespace dc
{
using json = nlohmann::json;
class Graph;

class Module
{
public:
	Module() = default;
	virtual ~Module() = default;

	void setBufferSize(size_t bufferSize);
	void setSampleRate(double sampleRate) { _sampleRate = sampleRate; }
	void process(size_t rev);
	AudioBuffer& getAudioOutputBuffer();
	ControlBuffer& getControlOutputBuffer();

	// Audio connection
	size_t getNumAudioInputs() const { return _audioInputs.size(); }
	void setNumAudioInputs(size_t numInputs);
	size_t getNumAudioOutputs() const { return _audioOutputs.size(); }
	void setNumAudioOutputs(size_t numOutputs);

	static bool connectAudio(Module* from, size_t fromIdx, Module* to, size_t toIdx);

	// Control connection
	size_t getNumControlInputs() const { return _controlInputs.size(); }
	void setNumControlInputs(size_t numInputs);
	size_t getNumControlOutputs() const { return _controlOutputs.size(); }
	void setNumControlOutputs(size_t numOutputs);

	static bool connectControl(Module* from, size_t fromIdx, Module* to, size_t toIdx);

	// serialization
	json toJson() const;
	static std::unique_ptr<Module> createFromJson(const json& j);
	void fromJson(const json& j);
	static void updateConnectionsFromJson(const json& j, Graph& parentGraph);

	// the id of this Module instance in its parent graph
	size_t id = 0;

protected:
	virtual void onProcess() {}
	virtual void onRefreshAudioBuffers() {}
	virtual void onRefreshControlBuffers() {}

	void pushControlMessage(ControlMessage message, size_t outputIndex);

	virtual nlohmann::json toJsonInternal() const = 0;
	virtual void fromJsonInternal(const nlohmann::json& j) = 0;

	virtual std::string getModuleIdForInstance() const = 0;

	double _sampleRate = 0;
	AudioBuffer _audioBuffer;
	ControlBuffer _controlBuffer;

private:
	struct AudioOutput final
	{
		AudioOutput(Module& parent, size_t index) : parent(parent), index(index) {}

		Module& parent;
		size_t index;
	};

	struct AudioInput final
	{
		explicit AudioInput(Module& parent) : parent(parent) {}

		Module& parent;
		std::vector<std::weak_ptr<AudioOutput>> outputs;
	};

	struct ControlOutput final
	{
		ControlOutput(Module& parent, size_t index) : parent(parent), index(index) {}

		Module& parent;
		size_t index;
	};

	struct ControlInput final
	{
		explicit ControlInput(Module& parent) : parent(parent) {}

		Module& parent;
		std::vector<std::weak_ptr<ControlOutput>> outputs;
	};

	void refreshAudioBuffers(size_t numSamples);
	void refreshControlBuffers();

	std::vector<std::unique_ptr<AudioInput>> _audioInputs;
	std::vector<std::shared_ptr<AudioOutput>> _audioOutputs;
	std::vector<std::unique_ptr<ControlInput>> _controlInputs;
	std::vector<std::shared_ptr<ControlOutput>> _controlOutputs;
	size_t _rev = 0;
};

/*
 * Use this to create a module when you can't do it explicitly (such as when deserializing)
 * NOTE: If you make a new Module, you'll need to register it with the factory.
 */
class ModuleFactory
{
public:
	using ModuleCreateMethod = std::function<std::unique_ptr<Module>()>;

	ModuleFactory() = delete;

	static bool registerModule(const std::string& name, ModuleCreateMethod moduleCreateMethod);
	static std::unique_ptr<Module> create(const std::string& name);

private:
	static std::map<std::string, ModuleCreateMethod> _moduleCreateMethods;
};
}
