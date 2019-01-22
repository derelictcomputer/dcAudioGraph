#include "Transport.h"
#include <algorithm>

void dc::Transport::reset()
{
	_elapsedSamples = 0;
	_elapsedSeconds = 0.0;
	_elapsedQuarters = 0.0;
	_elapsedQuartersInBar = 0.0;
}

void dc::Transport::setTempo(double tempo)
{
	_tempo = std::max(1.0, tempo);
}

void dc::Transport::setTimeSignatureNumerator(uint8_t numerator)
{
	_timeSignatureNumerator = std::max(static_cast<uint8_t>(1), numerator);
}

void dc::Transport::setTimeSignatureDenominator(uint8_t denominator)
{
	_timeSignatureDenominator = std::max(static_cast<uint8_t>(1), denominator);
}

void dc::Transport::update(size_t numSamples, double sampleRate)
{
	if (!_playing)
	{
		return;
	}

	_elapsedSamples.fetch_add(numSamples);
	_elapsedSeconds = _elapsedSeconds + (numSamples / sampleRate);

	// update the music timing info
	{
		const double tempo = _tempo.load();
		const uint8_t timeSigNum = _timeSignatureNumerator.load();
		const uint8_t timeSigDen = _timeSignatureDenominator.load();

		const double quartersPerSecond = tempo / 60.0;
		const double quartersPerSample = quartersPerSecond / sampleRate;
		const double quartersInBuffer = quartersPerSample * numSamples;

		_elapsedQuarters = _elapsedQuarters + quartersInBuffer;

		const double quartersPerBar = timeSigNum * 4.0 / timeSigDen;
		double qInBar = _elapsedQuartersInBar + quartersInBuffer;

		while (qInBar > quartersPerBar)
		{
			_elapsedBars.fetch_add(1);
			qInBar -= quartersPerBar;
		}

		_elapsedQuartersInBar = qInBar;
	}
}
