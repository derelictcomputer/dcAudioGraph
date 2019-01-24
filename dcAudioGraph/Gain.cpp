#include "Gain.h"

void dc::Gain::GainProcessor::process(AudioBuffer& audioBuffer, ControlBuffer& controlBuffer)
{
	const size_t nChannels = audioBuffer.getNumChannels();
	const size_t nSamples = audioBuffer.getNumSamples();
	size_t lastSample = 0;

	// knob info
	float knobValue = _lastKnobValue;
	float knobInc = 0;

	// control input info
	float controlValue = _lastControlValue;

	if (_updateGainParam)
	{
		knobInc = (getParamValueNormalized(0) - knobValue) / nSamples;
	}

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

			for (size_t cIdx = 0; cIdx < nChannels; ++cIdx)
			{
				auto* cPtr = audioBuffer.getChannelPointer(cIdx);
				for (; lastSample < msg.sampleOffset; ++lastSample)
				{
					const float combinedValue = knobValue + controlValue * (1.0f - knobValue);
					const float gain = getRawValueFromNormalized(0, combinedValue);
					cPtr[lastSample] *= gain;
					knobValue += knobInc;
					controlValue += controlInc;
				}
			}
		}
	}

	if (lastSample < nSamples)
	{
		for (size_t cIdx = 0; cIdx < nChannels; ++cIdx)
		{
			auto* cPtr = audioBuffer.getChannelPointer(cIdx);
			for (; lastSample < nSamples; ++lastSample)
			{
				const float combinedValue = knobValue + controlValue * (1.0f - knobValue);
				const float gain = getRawValueFromNormalized(0, combinedValue);
				cPtr[lastSample] *= gain;
				knobValue += knobInc;
			}
		}
	}

	_lastKnobValue = knobValue;
	_lastControlValue = controlValue;
}

void dc::Gain::GainProcessor::paramValueChanged(size_t)
{
	_updateGainParam = true;
}

dc::Gain::Gain() : Module(std::make_unique<GainProcessor>())
{
	setNumIo(Audio | Input | Output, 1);
	addParam("gain", "Gain", ParamRange(0.0f, 1.0f, 0.0f), true, true);
	setParamValue("gain", 1.0f);
}
