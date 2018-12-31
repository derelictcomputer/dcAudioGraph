/*
  ==============================================================================

    GraphModule.cpp
    Created: 27 Dec 2018 7:34:18pm
    Author:  Charlie Huguenard

  ==============================================================================
*/

#include "GraphModule.h"

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
