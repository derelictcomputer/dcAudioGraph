/*
  ==============================================================================

    AudioBuffer.cpp
    Created: 27 Dec 2018 3:19:06pm
    Author:  Charlie Huguenard

  ==============================================================================
*/

#include "AudioBuffer.h"
#include <algorithm>

dc::AudioBuffer::AudioBuffer(size_t numSamples, size_t numChannels)
{
	resize(numSamples, numChannels);
}

dc::AudioBuffer::~AudioBuffer()
{
	delete[] _data;
}

void dc::AudioBuffer::resize(size_t numSamples, size_t numChannels)
{
	// filter redundant calls
	if (numSamples == _numSamples && numChannels == _numChannels)
	{
		return;
	}

	// if we're downsizing or keeping the same total size, just change the counts
	if (numSamples * numChannels <= _numSamples * _numChannels)
	{
		_numSamples = numSamples;
		_numChannels = numChannels;
		return;
	}

	_numSamples = numSamples;
	_numChannels = numChannels;

	// free the old data
	delete[] _data;

	// allocate the new data
	_data = new float[_numSamples * _numChannels];

	// zero the new data
	// TODO: it might be useful to allow keeping the old data
	zero();
}

void dc::AudioBuffer::fill(float value)
{
	if (nullptr != _data)
	{
		std::fill(_data, _data + sizeof(_data), value);
	}
}

void dc::AudioBuffer::fill(size_t channel, float value)
{
	if (float* start = getChannelPointer(channel))
	{
		std::fill(start, start + _numSamples, value);
	}
}

void dc::AudioBuffer::zero()
{
	fill(0.0f);
}

void dc::AudioBuffer::zero(size_t channel)
{
	fill(channel, 0.0f);
}

void dc::AudioBuffer::copyFrom(const AudioBuffer& other, bool allowResize)
{
	if (allowResize)
	{
		resize(other._numSamples, other._numChannels);
	}

	const size_t numChannelsToCopy = std::min(_numChannels, other._numChannels);
	for (size_t cIdx = 0; cIdx < numChannelsToCopy; ++cIdx)
	{
		copyFrom(other, cIdx, cIdx);
	}
}

void dc::AudioBuffer::copyFrom(const AudioBuffer& other, size_t fromChannel, size_t toChannel)
{
	if (fromChannel < other.getNumChannels() && toChannel < _numChannels)
	{
		const size_t numSamplesToCopy = std::min(_numSamples, other._numSamples);
		float* fromStart = other._data + fromChannel * other._numSamples;
		float* fromEnd = fromStart + numSamplesToCopy;
		float* toStart = _data + toChannel * _numSamples;
		std::copy(fromStart, fromEnd, toStart);
	}
}

void dc::AudioBuffer::addFrom(const AudioBuffer& other)
{
	const size_t numChannelsToAdd = std::min(_numChannels, other._numChannels);
	for (size_t cIdx = 0; cIdx < numChannelsToAdd; ++cIdx)
	{
		addFrom(other, cIdx, cIdx);
	}
}

void dc::AudioBuffer::addFrom(const AudioBuffer& other, size_t fromChannel, size_t toChannel)
{
	if (fromChannel < other.getNumChannels() && toChannel < _numChannels)
	{
		const size_t numSamplesToAdd = std::min(_numSamples, other._numSamples);
		float* fromPtr = other._data + fromChannel * other._numSamples;
		float* toPtr = _data + toChannel * _numSamples;
		for (size_t sIdx = 0; sIdx < numSamplesToAdd; ++sIdx)
		{
			toPtr[sIdx] = fromPtr[sIdx];
		}
	}
}

void dc::AudioBuffer::fromInterleaved(const float *buffer, size_t numSamples, size_t numChannels, bool allowResize)
{
    if (allowResize && numSamples != _numSamples && numChannels != _numChannels)
    {
        resize(numSamples, numChannels);
    }
    for (size_t sIdx = 0; sIdx < _numSamples; ++sIdx)
    {
        if (sIdx >= numSamples)
        {
            break;
        }

        for (size_t cIdx = 0; cIdx < _numChannels; ++cIdx)
        {
            if (cIdx >= numChannels)
            {
                break;
            }
            _data[sIdx + cIdx * _numSamples] = buffer[cIdx + sIdx * numChannels];
        }
    }
}

void dc::AudioBuffer::toInterleaved(float* buffer, size_t numSamples, size_t numChannels)
{
	for (size_t cIdx = 0; cIdx < numChannels; ++cIdx)
	{
		for (size_t sIdx = 0; sIdx < numSamples; ++sIdx)
		{
			if (cIdx >= _numChannels || sIdx >= _numSamples)
			{
				buffer[cIdx + sIdx * numChannels] = 0;
			}
			else
			{
				buffer[cIdx + sIdx * numChannels] = _data[sIdx + cIdx * _numSamples];
			}
		}
	}
}

float* dc::AudioBuffer::getChannelPointer(size_t channel)
{
	if (channel < _numChannels)
	{
		return _data + channel * _numSamples;
	}

	return nullptr;
}
