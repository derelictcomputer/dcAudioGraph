/*
  ==============================================================================

    ModuleBase.cpp
    Created: 25 Dec 2018 12:25:16pm
    Author:  charl

  ==============================================================================
*/

#include "Module.h"
#include <algorithm>

void dc::Module::init(size_t bufferSize)
{
	refreshIo(bufferSize);
	onInit(bufferSize);
}

void dc::Module::teardown()
{
	onTeardown();
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
	if (_processBuffer.getNumChannels() < _audioInputs.size())
	{
		return;
	}

	// pull audio from inputs to process buffer
	for (size_t cIdx = 0; cIdx < _processBuffer.getNumChannels(); ++cIdx)
	{
		// clear input buffer
		_processBuffer.zero(cIdx);

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
					_processBuffer.addFrom(aOut->parent.getOutputBuffer(), aOut->index, cIdx);
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
	return _processBuffer;
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
	refreshIo(_processBuffer.getNumSamples());
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
	refreshIo(_processBuffer.getNumSamples());
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

void dc::Module::refreshIo(size_t bufferSize)
{
	_processBuffer.resize(bufferSize, std::max(_audioInputs.size(), _audioOutputs.size()));
	onRefreshIo(bufferSize);
}
