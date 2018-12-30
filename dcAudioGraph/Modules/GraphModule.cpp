/*
  ==============================================================================

    GraphModule.cpp
    Created: 27 Dec 2018 7:34:18pm
    Author:  Charlie Huguenard

  ==============================================================================
*/

#include "GraphModule.h"

void dc::GraphModule::onInit(size_t bufferSize)
{
	_graph.init(bufferSize);
}

void dc::GraphModule::onTeardown()
{
	_graph.teardown();
}

void dc::GraphModule::onProcess()
{
	// we know that the Graph processes the input buffer first,
	// then copies the result to the output buffer, so we can just
	// pass the process buffer in as both
	_graph.process(_processBuffer, _processBuffer);
}

void dc::GraphModule::onRefreshIo(size_t /*bufferSize*/)
{
	_graph.setNumAudioInputs(getNumAudioInputs());
	_graph.setNumAudioOutputs(getNumAudioOutputs());
}
