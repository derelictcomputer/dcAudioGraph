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
	_graph.process(_audioBuffer, _controlBuffer);
}

void dc::GraphModule::onRefreshAudioBuffers()
{
	_graph.init(_audioBuffer.getNumSamples(), _sampleRate);
	_graph.setNumAudioInputs(getNumAudioInputs());
	_graph.setNumAudioOutputs(getNumAudioOutputs());
}
