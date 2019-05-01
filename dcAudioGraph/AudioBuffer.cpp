#include <algorithm>
#include <cmath>
#include <cstring>
#include "AudioBuffer.h"

dc::AudioBuffer::AudioBuffer(size_t numSamples, size_t numChannels)
{
  resize(numSamples, numChannels);
}

dc::AudioBuffer::AudioBuffer(const AudioBuffer& other) : AudioBuffer(other._numSamples, other._numChannels)
{
  copyFrom(other, false);
}

dc::AudioBuffer& dc::AudioBuffer::operator=(const AudioBuffer& other)
{
  if (this != &other)
  {
    copyFrom(other, true);
  }
  return *this;
}

dc::AudioBuffer::~AudioBuffer()
{
  free(_data);
}

void dc::AudioBuffer::resize(size_t numSamples, size_t numChannels)
{
  // filter redundant calls
  if (numSamples == _numSamples && numChannels == _numChannels)
  {
    return;
  }

  // if we're downsizing or keeping the same total size, just change the counts
  if (numSamples * numChannels <= _allocatedSize)
  {
    _numSamples = numSamples;
    _numChannels = numChannels;
    return;
  }

  // free the old data
  free(_data);

  // set the new size
  _numSamples = numSamples;
  _numChannels = numChannels;

  // allocate the new data
  _allocatedSize = _numSamples * _numChannels;
  _data = static_cast<float*>(malloc(_allocatedSize * sizeof(float)));
}

void dc::AudioBuffer::fill(float value)
{
  for (size_t i = 0; i < _allocatedSize; ++i)
  {
    _data[i] = value;
  }
}

void dc::AudioBuffer::fill(size_t channel, float value)
{
  if (float* start = getChannelPointer(channel))
  {
    for (size_t i = 0; i < _numSamples; ++i)
    {
      start[i] = value;
    }
  }
}

void dc::AudioBuffer::zero()
{
  memset(_data, 0, _allocatedSize * sizeof(float));
}

void dc::AudioBuffer::zero(size_t channel)
{
  if (float* start = getChannelPointer(channel))
  {
    memset(start, 0, _numSamples * sizeof(float));
  }
}

void dc::AudioBuffer::copyFrom(const AudioBuffer& other, bool allowResize)
{
  if (allowResize)
  {
    resize(other._numSamples, other._numChannels);
  }

  // if buffers are the same size, shortcut
  if (_numSamples == other._numSamples && _numChannels == other._numChannels)
  {
    memcpy(_data, other._data, _allocatedSize * sizeof(float));
  }
  else
  {
    const size_t numChannelsToCopy = std::min(_numChannels, other._numChannels);

    for (size_t cIdx = 0; cIdx < numChannelsToCopy; ++cIdx)
    {
      copyFrom(other, cIdx, cIdx);
    }
  }
}

void dc::AudioBuffer::copyFrom(const AudioBuffer& other, size_t fromChannel, size_t toChannel)
{
  if (fromChannel < other.getNumChannels() && toChannel < _numChannels)
  {
    const size_t numSamplesToCopy = std::min(_numSamples, other._numSamples);
    float* from = other._data + fromChannel * other._numSamples;
    float* to = _data + toChannel * _numSamples;
    memcpy(to, from, numSamplesToCopy * sizeof(float));
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
      toPtr[sIdx] += fromPtr[sIdx];
    }
  }
}

void dc::AudioBuffer::applyGain(float gain)
{
  for (size_t i = 0; i < _allocatedSize; ++i)
  {
    _data[i] *= gain;
  }
}

void dc::AudioBuffer::applyGain(size_t channel, float gain)
{
  if (auto* cPtr = getChannelPointer(channel))
  {
    for (size_t sIdx = 0; sIdx < _numSamples; ++sIdx)
    {
      cPtr[sIdx] *= gain;
    }
  }
}

void dc::AudioBuffer::fromInterleaved(const float* buffer, size_t numSamples, size_t numChannels, bool allowResize)
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

float dc::AudioBuffer::getRms(size_t channel) const
{
  if (channel >= _numChannels)
  {
    return 0;
  }

  auto* cPtr = _data + channel * _numSamples;
  float sum = 0.0f;
  for (size_t sIdx = 0; sIdx < _numSamples; ++sIdx)
  {
    const float sample = cPtr[sIdx];
    sum += sample * sample;
  }
  return std::sqrt(sum / _numSamples);
}

float dc::AudioBuffer::getPeak(size_t channel) const
{
  if (channel >= _numChannels)
  {
    return 0;
  }

  auto* cPtr = _data + channel * _numSamples;
  float peak = 0;
  for (size_t sIdx = 0; sIdx < _numSamples; ++sIdx)
  {
    const float val = std::abs(cPtr[sIdx]);
    if (val > peak)
    {
      peak = val;
    }
  }
  return peak;
}
