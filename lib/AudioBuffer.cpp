/*
  ==============================================================================

    AudioBuffer.cpp
    Created: 27 Dec 2018 3:19:06pm
    Author:  charl

  ==============================================================================
*/

#include "AudioBuffer.h"

void dc::AudioBuffer::resize(size_t numSamples, size_t numChannels)
{
	_data.resize(numSamples * numChannels);
	_numSamples = numSamples;
	_numChannels = numChannels;
}

void dc::AudioBuffer::zero(size_t channel)
{
	if (channel < _numChannels)
	{
		const auto begin = _data.begin() + channel * _numSamples;
		const auto end = begin + _numSamples;
		std::fill(begin, end, 0.0f);
	}
}

void dc::AudioBuffer::addFrom(const AudioBuffer& other, size_t fromChannel, size_t toChannel)
{
	if (fromChannel < other.getNumChannels() && toChannel < _numChannels)
	{
		for (size_t sIdx = 0; sIdx < other.getNumSamples(); ++sIdx)
		{
			if (sIdx == _numSamples)
			{
				break;
			}
			const size_t myIdx = sIdx + _numSamples * toChannel;
			const size_t otherIdx = sIdx + other.getNumSamples() * fromChannel;
			_data[myIdx] += other._data[otherIdx];
		}
	}
}

void dc::AudioBuffer::copyFrom(const AudioBuffer& other, size_t fromChannel, size_t toChannel)
{
	if (fromChannel < other.getNumChannels() && toChannel < _numChannels)
	{
		for (size_t sIdx = 0; sIdx < other.getNumSamples(); ++sIdx)
		{
			if (sIdx == _numSamples)
			{
				break;
			}
			const size_t myIdx = sIdx + _numSamples * toChannel;
			const size_t otherIdx = sIdx + other.getNumSamples() * fromChannel;
			_data[myIdx] = other._data[otherIdx];
		}
	}
}

float* dc::AudioBuffer::getChannelPointer(size_t channel)
{
	if (channel < _numChannels)
	{
		return &_data.data()[channel * _numSamples];
	}
	return nullptr;
}
