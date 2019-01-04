/*
  ==============================================================================

    GraphModule.cpp
    Created: 27 Dec 2018 7:34:18pm
    Author:  Charlie Huguenard

  ==============================================================================
*/

#include "GraphModule.h"

using json = nlohmann::json;

void dc::GraphModule::onProcess()
{
	// we know that the Graph processes the input buffer first,
	// then copies the result to the output buffer, so we can just
	// pass the process buffer in as both
	_graph.process(_audioBuffer, _audioBuffer);
}

void dc::GraphModule::onRefreshAudioBuffers()
{
	_graph.init(_audioBuffer.getNumSamples(), _sampleRate);
	_graph.setNumAudioInputs(getNumAudioInputs());
	_graph.setNumAudioOutputs(getNumAudioOutputs());
}

// serialization keys
const std::string S_GRAPH = "graph";

json dc::GraphModule::toJsonInternal() const
{
	json j;
	j[S_GRAPH] = _graph.toJson();
	return j;
}

void dc::GraphModule::fromJsonInternal(const json& j)
{
	_graph.fromJson(j[S_GRAPH]);
}
