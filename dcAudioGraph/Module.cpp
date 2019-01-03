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
	refreshBuffers(bufferSize);
}

void dc::Module::process(size_t rev)
{
	// check revision, if already processed, return
	if (rev == _rev)
	{
		return;
	}
	_rev = rev;

	// we assume _processBuffer has enough channels for all the inputs,
	// but let's check anyway
	if (_buffer.getNumChannels() < _audioInputs.size())
	{
		return;
	}

	// pull audio from inputs to process buffer
	for (size_t cIdx = 0; cIdx < _buffer.getNumChannels(); ++cIdx)
	{
		// clear input buffer
		_buffer.zero(cIdx);

		// if we have an input for this channel, pull it into the process buffer
		if (cIdx < _audioInputs.size())
		{
			auto* aIn = _audioInputs[cIdx].get();
			size_t oIdx = 0;
			while (oIdx < aIn->outputs.size())
			{
				if (auto aOut = aIn->outputs[oIdx].lock())
				{
					// process upstream modules
					aOut->parent.process(rev);
					// sum the audio from each input's connections
					_buffer.addFrom(aOut->parent.getOutputBuffer(), aOut->index, cIdx);
					++oIdx;
				}
				// if output doesn't exist anymore, remove it
				else
				{
					aIn->outputs.erase(aIn->outputs.begin() + oIdx);
				}
			}
		}
	}

	// process the audio
	onProcess();
}

dc::AudioBuffer& dc::Module::getOutputBuffer()
{
	return _buffer;
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
	refreshBuffers(_buffer.getNumSamples());
}

void dc::Module::setNumAudioOutputs(size_t numOutputs)
{
	while (numOutputs < _audioOutputs.size())
	{
		_audioOutputs.pop_back();
	}
	while (numOutputs > _audioOutputs.size())
	{
		_audioOutputs.push_back(std::make_unique<AudioOutput>(*this, _audioOutputs.size()));
	}
	refreshBuffers(_buffer.getNumSamples());
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

json dc::Module::toJson() const
{
	json j;
	j["_moduleId"] = getModuleIdForInstance();
	j["_graphId"] = id;
	
	// audio I/O
	j["numAudioInputs"] = getNumAudioInputs();
	j["numAudioOutputs"] = getNumAudioOutputs();

	// audio input connections
	auto ic = json::array();
	for (size_t i = 0; i < getNumAudioInputs(); ++i)
	{
		for (auto& output : _audioInputs[i]->outputs)
		{
			if (auto oP = output.lock())
			{
				ic.push_back({ 
					{"inputIndex", i}, 
					{"outputGraphId", oP->parent.id}, 
					{"outputIndex", oP->index} 
				});
			}
		}
	}
	j["audioInputConnections"] = ic;

	// settings for concrete modules
	j["settings"] = toJsonInternal();

	return j;
}

std::unique_ptr<dc::Module> dc::Module::createFromJson(const json& j)
{
	std::string moduleId = j["_moduleId"].get<std::string>();
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
	id = j["_graphId"].get<size_t>();
	
	// audio inputs
	setNumAudioInputs(j["numAudioInputs"].get<size_t>());
	setNumAudioOutputs(j["numAudioOutputs"].get<size_t>());

	// NOTE: we will make the connections after all nodes have been configured for the parent graph

	fromJsonInternal(j["settings"]);
}

void dc::Module::updateConnectionsFromJson(const json& j, Graph& parentGraph)
{
	auto connections = j["audioInputConnections"];

	for (auto c : connections)
	{
		const size_t inputIdx = c["inputIndex"].get<size_t>();
		const size_t outputModuleId = c["outputGraphId"].get<size_t>();
		const size_t outputIdx = c["outputIndex"].get<size_t>();
		if (auto* other = parentGraph.getModuleById(outputModuleId))
		{
			connectAudio(other, outputIdx, this, inputIdx);
		}
	}
}

void dc::Module::refreshBuffers(size_t numSamples)
{
	_buffer.resize(numSamples, std::max(_audioInputs.size(), _audioOutputs.size()));
	onRefreshBuffers();
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
