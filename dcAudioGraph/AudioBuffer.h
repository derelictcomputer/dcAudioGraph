/*
 * a de-interleaved audio buffer
 * Note: this class is not thread-safe, so if you're operating on one in multiple threads,
 * be sure to implement your own synchronization
 */

#pragma once

#include <cstddef>

namespace dc
{
class AudioBuffer final
{
public:
  // creates an empty buffer that's not useful until resize() is called
  AudioBuffer() = default;

  // creates a buffer with a number of samples and channels
  // Note: the buffer will be filled with garbage. Clear it out before you use it.
  AudioBuffer(size_t numSamples, size_t numChannels);

  // copying buffers
  AudioBuffer(const AudioBuffer& other);

  AudioBuffer& operator=(const AudioBuffer& other);

  // don't need these for now
  AudioBuffer(AudioBuffer&& other) = delete;

  AudioBuffer& operator=(AudioBuffer&& other) = delete;

  ~AudioBuffer();

  // get the number of samples per channel
  size_t getNumSamples() const { return _numSamples; }

  // get the number of channels in the buffer
  size_t getNumChannels() const { return _numChannels; }

  // set the number of samples and channels in the buffer
  // Note: the buffer will be filled with garbage. Clear it out before you use it.
  // Note: this will reallocate the underlying data if the total size increases,
  // so don't call this on the audio thread unless you're sure you're downsizing
  void resize(size_t numSamples, size_t numChannels);

  // fill the buffer with a value
  void fill(float value);

  // fill a channel of the buffer with a value
  void fill(size_t channel, float value);

  // fill the buffer with zeros
  void zero();

  // fill a channel of the buffer with zeros
  void zero(size_t channel);

  // copy the contents of another buffer into this one, and optionally resize it
  // Note: resizing will reallocate the underlying data if the total size increases,
  // so don't call this on the audio thread unless you're sure you're downsizing
  void copyFrom(const AudioBuffer& other, bool allowResize);

  // copy the contents of a channel to a channel in this buffer,
  // but do not attempt to resize
  void copyFrom(const AudioBuffer& other, size_t fromChannel, size_t toChannel);

  // add the contents of another buffer to this one
  void addFrom(const AudioBuffer& other);

  // add the contents of a channel to a channel in this buffer
  void addFrom(const AudioBuffer& other, size_t fromChannel, size_t toChannel);

  // apply gain to the whole buffer
  void applyGain(float gain);

  // apply gain to a channel
  void applyGain(size_t channel, float gain);

  // copy the contents of an interleaved raw buffer to this one, and optionally resize it
  // Note: resizing will reallocate the underlying data if the total size increases,
  // so don't call this on the audio thread unless you're sure you're downsizing
  void fromInterleaved(const float* buffer, size_t numSamples, size_t numChannels, bool allowResize);

  // copies the contents of this buffer to an interleaved raw buffer
  void toInterleaved(float* buffer, size_t numSamples, size_t numChannels);

  // Get the RMS level of a channel in the buffer
  float getRms(size_t channel) const;

  // Get the peak level of a channel in the buffer
  float getPeak(size_t channel) const;

  // get a pointer to a channel in this buffer
  // Note: if you want to iterate the whole buffer, just get channel 0
  float* getChannelPointer(size_t channel);

private:
  float* _data = nullptr;
  size_t _numSamples = 0;
  size_t _numChannels = 0;
  size_t _allocatedSize = 0;
};
}
