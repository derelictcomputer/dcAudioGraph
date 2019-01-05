/*
  ==============================================================================

    ModuleBase.cpp
    Created: 25 Dec 2018 12:25:16pm
    Author:  Charlie Huguenard

  ==============================================================================
*/

#include "Module.h"
#include "Graph.h"

using json = nlohmann::json;

void dc::Module::setBufferSize(size_t bufferSize)
{
	refreshAudioBuffers(bufferSize);
}

void dc::Module::process(size_t rev)
{
	// check revision, if already processed, return
	if (rev == _rev)
	{
		return;
	}
	_rev = rev;

	// pull in control data
	{
		_controlBuffer.clear();

		for (size_t i = 0; i < _controlBuffer.getNumChannels(); ++i)
		{
			if (i < _controlInputs.size())
			{
				// merge data if there are multiple connections
				for (auto& connectedOutput : _controlInputs[i]->outputs)
				{
					if (auto oPtr = connectedOutput.lock())
					{
						_controlBuffer.merge(oPtr->parent.getControlOutputBuffer(), oPtr->index, i);
					}
				}
			}
		}
	}

	// pull in audio data
	// Note: we assume there are enough channels in the temp buffer for the inputs
	// TODO: safety checks for debug builds
	{
		// clear the whole buffer
		_audioBuffer.zero();

		// pull audio from inputs to process buffer
		for (size_t cIdx = 0; cIdx < _audioInputs.size(); ++cIdx)
		{
			auto* aIn = _audioInputs[cIdx].get();

			// merge data from all connections
			for (auto& aOut : aIn->outputs)
			{
				if (auto outPtr = aOut.lock())
				{
					// process upstream modules
					outPtr->parent.process(rev);
					// sum the audio
					_audioBuffer.addFrom(outPtr->parent.getAudioOutputBuffer(), outPtr->index, cIdx);
				}
			}
		}
	}

	// process the audio
	onProcess();
}

dc::AudioBuffer& dc::Module::getAudioOutputBuffer()
{
	return _audioBuffer;
}

dc::ControlBuffer& dc::Module::getControlOutputBuffer()
{
	return _controlBuffer;
}

void dc::Module::setNumAudioInputs(size_t numInputs)
{
	while (numInputs < _audioInputs.size())
	{
		_audioInputs.pop_back();
	}
	while (numInputs > _audioInputs.size())
	{
		_audioInputs.push_back(std::make_unique<AudioInput>(*this));
	}
	refreshAudioBuffers(_audioBuffer.getNumSamples());
}

void dc::Module::setNumAudioOutputs(size_t numOutputs)
{
	while (numOutputs < _audioOutputs.size())
	{
		_audioOutputs.pop_back();
	}
	while (numOutputs > _audioOutputs.size())
	{
		_audioOutputs.push_back(std::make_shared<AudioOutput>(*this, _audioOutputs.size()));
	}
	refreshAudioBuffers(_audioBuffer.getNumSamples());
}

bool dc::Module::connectAudio(Module* from, size_t fromIdx, Module* to, size_t toIdx)
{
	if (nullptr != from && nullptr != to)
	{
		if (fromIdx < from->getNumAudioOutputs() && toIdx < to->getNumAudioInputs())
		{
			to->_audioInputs[toIdx]->outputs.emplace_back(from->_audioOutputs[fromIdx]);
			// TODO: check for dupes
			// TODO: check for loops
			return true;
		}
	}
	return false;
}

void dc::Module::setNumControlInputs(size_t numInputs)
{
	while (numInputs < _controlInputs.size())
	{
		_controlInputs.pop_back();
	}
	while (numInputs > _controlInputs.size())
	{
		_controlInputs.push_back(std::make_unique<ControlInput>(*this));
	}
	refreshControlBuffers();
}

void dc::Module::setNumControlOutputs(size_t numOutputs)
{
	while (numOutputs < _controlOutputs.size())
	{
		_controlOutputs.pop_back();
	}
	while (numOutputs > _controlOutputs.size())
	{
		_controlOutputs.push_back(std::make_shared<ControlOutput>(*this, _controlOutputs.size()));
	}
	refreshControlBuffers();
}

bool dc::Module::connectControl(Module* from, size_t fromIdx, Module* to, size_t toIdx)
{
	if (nullptr != from && nullptr != to)
	{
		if (fromIdx < from->getNumControlOutputs() && toIdx < to->getNumControlInputs())
		{
			to->_controlInputs[toIdx]->outputs.emplace_back(from->_controlOutputs[fromIdx]);
			// TODO: check for dupes
			// TODO: check for loops
			return true;
		}
	}
	return false;
}

void dc::Module::pushControlMessage(ControlMessage message, size_t outputIndex)
{
	_controlBuffer.insert(message, outputIndex);
}

// serialization keys
const std::string S_MODULE_ID = "_moduleId";
const std::string S_GRAPH_ID = "_graphId";
const std::string S_NUM_AUDIO_INPUTS = "numAudioInputs";
const std::string S_NUM_AUDIO_OUTPUTS = "numAudioOutputs";
const std::string S_AUDIO_INPUT_CONNECTIONS = "audioInputConnections";
const std::string S_AIC_INPUT_INDEX = "inputIndex";
const std::string S_AIC_OUTPUT_INDEX = "outputIndex";
const std::string S_AIC_OUTPUT_GRAPH_ID = "outputGraphId";
const std::string S_NUM_CONTROL_INPUTS = "numControlInputs";
const std::string S_NUM_CONTROL_OUTPUTS = "numControlOutputs";
const std::string S_CONTROL_INPUT_CONNECTIONS = "controlInputConnections";
const std::string S_CIC_INPUT_INDEX = "inputIndex";
const std::string S_CIC_OUTPUT_INDEX = "outputIndex";
const std::string S_CIC_OUTPUT_GRAPH_ID = "outputGraphId";
const std::string S_SETTINGS = "settings";

json dc::Module::toJson() const
{
	json j;
	j[S_MODULE_ID] = getModuleIdForInstance();
	j[S_GRAPH_ID] = id;
	
	// audio I/O
	{
		j[S_NUM_AUDIO_INPUTS] = getNumAudioInputs();
		j[S_NUM_AUDIO_OUTPUTS] = getNumAudioOutputs();

		// audio input connections
		auto ic = json::array();
		for (size_t i = 0; i < getNumAudioInputs(); ++i)
		{
			for (auto& output : _audioInputs[i]->outputs)
			{
				if (auto oP = output.lock())
				{
					ic.push_back({
						{S_AIC_INPUT_INDEX, i},
						{S_AIC_OUTPUT_GRAPH_ID, oP->parent.id},
						{S_AIC_OUTPUT_INDEX, oP->index}
						});
				}
			}
		}
		j[S_AUDIO_INPUT_CONNECTIONS] = ic;
	}

	// control I/O
	{
		j[S_NUM_CONTROL_INPUTS] = getNumControlInputs();
		j[S_NUM_CONTROL_OUTPUTS] = getNumControlOutputs();

		// control input connections
		auto ic = json::array();
		for (size_t i = 0; i < getNumControlInputs(); ++i)
		{
			for (auto& output : _audioInputs[i]->outputs)
			{
				if (auto oP = output.lock())
				{
					ic.push_back({
						{S_CIC_INPUT_INDEX, i},
						{S_CIC_OUTPUT_GRAPH_ID, oP->parent.id},
						{S_CIC_OUTPUT_INDEX, oP->index}
					});
				}
			}
		}
		j[S_CONTROL_INPUT_CONNECTIONS] = ic;
	}

	// settings for concrete modules
	j[S_SETTINGS] = toJsonInternal();

	return j;
}

std::unique_ptr<dc::Module> dc::Module::createFromJson(const json& j)
{
	const std::string moduleId = j[S_MODULE_ID].get<std::string>();
	auto instance = ModuleFactory::create(moduleId);
	if (nullptr != instance)
	{
		instance->fromJson(j);
	}
	return instance;
}

void dc::Module::fromJson(const json& j)
{
	// do the common module stuff first, in case the specific config depends on that
	id = j[S_GRAPH_ID].get<size_t>();
	
	// audio I/O
	setNumAudioInputs(j[S_NUM_AUDIO_INPUTS].get<size_t>());
	setNumAudioOutputs(j[S_NUM_AUDIO_OUTPUTS].get<size_t>());

	// control I/O
	setNumControlInputs(j[S_NUM_CONTROL_INPUTS].get<size_t>());
	setNumControlOutputs(j[S_NUM_CONTROL_OUTPUTS].get<size_t>());

	// NOTE: we will make the connections after all nodes have been configured for the parent graph

	fromJsonInternal(j[S_SETTINGS]);
}

void dc::Module::updateConnectionsFromJson(const json& j, Graph& parentGraph)
{
	const size_t sourceId = j[S_GRAPH_ID].get<size_t>();

	if (auto* source = parentGraph.getModuleById(sourceId))
	{
		// audio connections
		{
			auto connections = j[S_AUDIO_INPUT_CONNECTIONS];

			for (auto c : connections)
			{
				const size_t inputIdx = c[S_AIC_INPUT_INDEX].get<size_t>();
				const size_t outputModuleId = c[S_AIC_OUTPUT_GRAPH_ID].get<size_t>();
				const size_t outputIdx = c[S_AIC_OUTPUT_INDEX].get<size_t>();
				if (auto* other = parentGraph.getModuleById(outputModuleId))
				{
					connectAudio(other, outputIdx, source, inputIdx);
				}
			}
		}
		// control connections
		{
			auto connections = j[S_CONTROL_INPUT_CONNECTIONS];

			for (auto c : connections)
			{
				const size_t inputIdx = c[S_CIC_INPUT_INDEX].get<size_t>();
				const size_t outputModuleId = c[S_CIC_OUTPUT_GRAPH_ID].get<size_t>();
				const size_t outputIdx = c[S_CIC_OUTPUT_INDEX].get<size_t>();
				if (auto* other = parentGraph.getModuleById(outputModuleId))
				{
					connectControl(other, outputIdx, source, inputIdx);
				}
			}
		}
	}
}

void dc::Module::refreshAudioBuffers(size_t numSamples)
{
	_audioBuffer.resize(numSamples, std::max(_audioInputs.size(), _audioOutputs.size()));
	onRefreshAudioBuffers();
}

void dc::Module::refreshControlBuffers()
{
	_controlBuffer.setNumChannels(std::max(_controlInputs.size(), _controlOutputs.size()));
	onRefreshControlBuffers();
}

std::map<std::string, dc::ModuleFactory::ModuleCreateMethod> dc::ModuleFactory::_moduleCreateMethods;

bool dc::ModuleFactory::registerModule(const std::string& name, ModuleCreateMethod moduleCreateMethod)
{
	const auto it = _moduleCreateMethods.find(name);
	if (it == _moduleCreateMethods.end())
	{
		_moduleCreateMethods[name] = moduleCreateMethod;
		return true;
	}
	return false;
}

std::unique_ptr<dc::Module> dc::ModuleFactory::create(const std::string& name)
{
	const auto it = _moduleCreateMethods.find(name);
	if (it == _moduleCreateMethods.end())
	{
		return nullptr;
	}
	return it->second();
}
