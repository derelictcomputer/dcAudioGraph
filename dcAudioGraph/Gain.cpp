#include "Gain.h"

dc::Gain::GainProcessor::GainProcessor()
{
	_gainBuffer.resize(MODULE_DEFAULT_MAX_BLOCK_SIZE, 1);
}

void dc::Gain::GainProcessor::process(AudioBuffer& audioBuffer, ControlBuffer& controlBuffer)
{
	const size_t nChannels = audioBuffer.getNumChannels();
	const size_t nSamples = audioBuffer.getNumSamples();

	// refresh the gain buffer if we've changed the gain knob or got a control input
	const bool hasMessages = controlBuffer.getNumMessages(0) > 0;
	if (_updateGainParam || hasMessages)
	{
		size_t lastSample = 0;

		// knob info
		float knobValue = _lastKnobValue;
		float knobInc = 0;

		// control input info
		float controlValue = hasMessages ? _lastControlValue : 0.0f;

		if (_updateGainParam)
		{
			knobInc = (getParamValueNormalized(0) - knobValue) / nSamples;
			_updateGainParam = false;
		}

        // do the gain buffer filling
		{
			auto* cPtr = _gainBuffer.getChannelPointer(0);
			{
				auto it = controlBuffer.getIterator(0);
				ControlMessage msg;
				while (it.next(msg))
				{
					if (msg.sampleOffset >= nSamples)
					{
						continue;
					}

					const size_t sampleDelta = msg.sampleOffset - lastSample;
					const float controlInc = (msg.floatParam - controlValue) / sampleDelta;

					for (; lastSample < msg.sampleOffset; ++lastSample)
					{
						const float combinedValue = knobValue + controlValue * (1.0f - knobValue);
						const float gain = getRawValueFromNormalized(0, combinedValue);
						cPtr[lastSample] = gain;
						knobValue += knobInc;
						controlValue += controlInc;
					}
				}
			}

			if (lastSample < nSamples)
			{
				for (; lastSample < nSamples; ++lastSample)
				{
					const float combinedValue = knobValue + controlValue * (1.0f - knobValue);
					const float gain = getRawValueFromNormalized(0, combinedValue);
					cPtr[lastSample] = gain;
					knobValue += knobInc;
				}
			}
		}

		_lastKnobValue = knobValue;
		_lastControlValue = controlValue;

		// apply the gain buffer
		{
			auto* gbPtr = _gainBuffer.getChannelPointer(0);
			for (size_t cIdx = 0; cIdx < nChannels; ++cIdx)
			{
				auto* cPtr = audioBuffer.getChannelPointer(cIdx);
				for (size_t sIdx = 0; sIdx < nSamples; ++sIdx)
				{
					cPtr[sIdx] *= gbPtr[sIdx];
				}
			}
		}
	}
	// otherwise, just apply the gain knob's value
	else
	{
		audioBuffer.applyGain(getParamValue(0));
	}
}

void dc::Gain::GainProcessor::paramValueChanged(size_t)
{
	_updateGainParam = true;
}

void dc::Gain::GainProcessor::audioIoChanged()
{
	_gainBuffer.resize(getBlockSize(), 1);
}

dc::Gain::Gain() : Module(std::make_unique<GainProcessor>())
{
	setNumIo(Audio | Input | Output, 1);
	addParam("gain", "Gain", ParamRange(0.0f, 1.0f, 0.0f), true, true);
	setParamValue("gain", 1.0f);
}
