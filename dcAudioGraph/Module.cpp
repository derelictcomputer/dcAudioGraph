/*
  ==============================================================================

    ModuleBase.cpp
    Created: 25 Dec 2018 12:25:16pm
    Author:  Charlie Huguenard

  ==============================================================================
*/

#include "Module.h"
#include "Graph.h"
#include "ModuleFactory.h"

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
	j["moduleId"] = getModuleIdForInstance();
	j["graphId"] = id;
	
	// audio inputs
	j["audioInputs"] = json::array();
	for (auto& input : _audioInputs)
	{
		json i;
		for (auto& output : input->outputs)
		{
			if (auto oP = output.lock())
			{
				i.push_back({{"graphId", oP->parent.id}, {"index", oP->index}});
			}
		}
		j["audioInputs"].push_back(i);
	}
	
	// audio outputs
	j["numAudioOutputs"] = getNumAudioOutputs();

	// settings for concrete modules
	j["settings"] = toJsonInternal();

	return j;
}

std::unique_ptr<dc::Module> dc::Module::createFromJson(const json& j)
{
	std::string moduleId = j["moduleId"];
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
	id = j["graphId"].get<size_t>();
	// audio inputs
	auto inputInfo = j["audioInputs"];
	setNumAudioInputs(inputInfo.size());
	setNumAudioOutputs(j["numAudioOutputs"]);
	// NOTE: we will make the connections after all nodes have been configured for the parent graph

	fromJsonInternal(j);
}

void dc::Module::updateConnectionsFromJson(const json& j, Graph& parentGraph)
{
	auto inputs = j["audioInputs"];
	for (int i = 0; i < inputs.size(); ++i)
	{
		auto outputs = inputs[i];
		for (auto& output : outputs)
		{
			const size_t parentId = output["graphId"].get<size_t>();
			if (auto* module = parentGraph.getModuleById(parentId))
			{
				const size_t outputIdx = output["index"].get<size_t>();
				connectAudio(module, outputIdx, this, i);
			}
		}
	}
}

void dc::Module::refreshBuffers(size_t numSamples)
{
	_buffer.resize(numSamples, std::max(_audioInputs.size(), _audioOutputs.size()));
	onRefreshBuffers();
}
