/*
  ==============================================================================

    AudioBuffer.h
    Created: 27 Dec 2018 3:19:06pm
    Author:  Charlie Huguenard

  ==============================================================================
*/

#pragma once
#include <vector>

namespace dc
{
// a de-interleaved audio buffer
class AudioBuffer
{
public:
	AudioBuffer() = default;
	~AudioBuffer() = default;

	size_t getNumSamples() const { return _numSamples; }
	size_t getNumChannels() const { return _numChannels; }

	void resize(size_t numSamples, size_t numChannels);

	void zero(size_t channel);
	void addFrom(const AudioBuffer& other, size_t fromChannel, size_t toChannel);
	void copyFrom(const AudioBuffer& other, size_t fromChannel, size_t toChannel);

	void fromInterleaved(const float* buffer, size_t numSamples, size_t numChannels, bool allowResize);

	float* getChannelPointer(size_t channel);

private:
	std::vector<float> _data;
	size_t _numSamples = 0;
	size_t _numChannels = 0;
};
}
