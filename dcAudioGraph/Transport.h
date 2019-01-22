#pragma once
#include <cstdint>
#include <atomic>

namespace dc
{
class Transport
{
public:
	Transport() = default;

	// transport control
	void play(bool shouldPlay) { _playing = shouldPlay; }
	bool isPlaying() const { return _playing; }
	void reset();

	// music timing control
	double getTempo() const { return _tempo; }
	void setTempo(double tempo);
	uint8_t getTimeSignatureNumerator() const { return _timeSignatureNumerator; }
	void setTimeSignatureNumerator(uint8_t numerator);
	uint8_t getTimeSignatureDenominator() const { return _timeSignatureDenominator; }
	void setTimeSignatureDenominator(uint8_t denominator);

	// current time info getters
	size_t getElapsedSamples() const { return _elapsedSamples; }
	double getElapsedSeconds() const { return _elapsedSeconds; }
	double getElapsedQuarters() const { return _elapsedQuarters; }
	double getElapsedQuartersInBar() const { return _elapsedQuartersInBar; }
	size_t getElapsedBars() const { return _elapsedBars; }

	// run this every block to update the position
	void update(size_t numSamples, double sampleRate);

private:
	std::atomic<bool> _playing{ false };

	// music timing settings
	std::atomic<double> _tempo{ 120.0 };
	std::atomic<uint8_t> _timeSignatureNumerator{ 4 };
	std::atomic<uint8_t> _timeSignatureDenominator{ 4 };

	// current time info
	std::atomic<size_t> _elapsedSamples{ 0 };
	std::atomic<double> _elapsedSeconds{ 0.0 };
	std::atomic<double> _elapsedQuarters{ 0.0 };
	std::atomic<double> _elapsedQuartersInBar{ 0.0 };
	std::atomic<size_t> _elapsedBars{ 0 };
};
}
