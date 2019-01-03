/*
  ==============================================================================

    GraphModule.cpp
    Created: 27 Dec 2018 7:34:18pm
    Author:  Charlie Huguenard

  ==============================================================================
*/

#include "GraphModule.h"
#include "../ModuleFactory.h"

using json = nlohmann::json;

bool dc::GraphModule::_registered = ModuleFactory::registerModule(getModuleId(), createMethod);

void dc::GraphModule::onProcess()
{
	// we know that the Graph processes the input buffer first,
	// then copies the result to the output buffer, so we can just
	// pass the process buffer in as both
	_graph.process(_buffer, _buffer);
}

void dc::GraphModule::onRefreshBuffers()
{
	_graph.init(_buffer.getNumSamples(), _sampleRate);
	_graph.setNumAudioInputs(getNumAudioInputs());
	_graph.setNumAudioOutputs(getNumAudioOutputs());
}

json dc::GraphModule::toJsonInternal() const
{
	json j;
	j["graph"] = _graph.toJson();
	return j;
}

void dc::GraphModule::fromJsonInternal(const json& j)
{
	_graph.fromJson(j);
}
