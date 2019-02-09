/*
 * A single parameter in a module.
 * Add these to your Module classes to define control of your algorithm.
 */

#pragma once
#include <atomic>
#include <functional>

namespace dc
{
class ParamRange final
{
public:
	using GetNormFn = float(*)(float, float, float);
	using GetRawFn = float(*)(float, float, float);

	ParamRange() = default;
	ParamRange(float min, float max, float stepSize, float sliderSkew = 1.0f);
	ParamRange(float min, float max, float stepSize, GetNormFn getNormalized, GetRawFn getRaw, float sliderSkew = 1.0f);

	~ParamRange() = default;

	ParamRange(const ParamRange& other) = default;
	ParamRange& operator=(const ParamRange& other);
	// We don't need to move these. It's just 5 trivial copies.
	ParamRange(ParamRange&& other) = delete;
	ParamRange& operator=(ParamRange&& other) = delete;

	float getNormalized(float rawValue) const;
	float getRaw(float normalizedValue) const;
	float constrainRaw(float rawValue) const;

	float getMin() const { return _min; }
	float getMax() const { return _max; }
	float getStepSize() const { return _stepSize; }
	float getSliderSkew() const { return _sliderSkew; }

	// helpers
	static float clampToRange(float value, float min, float max);
	static float snapToStep(float value, float stepSize);

	// Conversion functions to use. Feel free to add your own wherever you'd like.
	static float getNormalizedLinear(float rawValue, float min, float max);
	static float getRawLinear(float normalizedValue, float min, float max);

private:
	float _min = 0.0f;
	float _max = 1.0f;
	float _stepSize = 0.0f;
	float _sliderSkew = 1.0f;
	GetNormFn _getNormalized = getNormalizedLinear;
	GetRawFn _getRaw = getRawLinear;
};

class ModuleParam final
{
public:
	ModuleParam(std::string id, std::string displayName, const ParamRange& range,
		bool serializable = false, int controlInputIndex = -1, float initialValue = 0.0f);

	ModuleParam(const ModuleParam& other);
	ModuleParam& operator=(const ModuleParam& other);

	std::string getId() const { return _id; }
	std::string getDisplayName() const { return _displayName; }

	bool isSerializable() const { return _serializable; }

	bool hasControlInput() const { return _controlInputIndex >= 0; }
	int getControlInputIndex() const { return _controlInputIndex; }

	float getNormalized() const;
	void setNormalized(float normalizedValue);
	float getRaw() const { return _value; }
	void setRaw(float rawValue);

	float getControlScale() const { return _controlScale.load(); }
	void setControlScale(float scale);

    float getControlInput() const { return _controlInput; }
    void setControlInput(float value);

    // for use by a Module's process() to get smoothed values with or without control combination
	void initSmoothing();
	void updateSmoothing(size_t numSamples);
	float getSmoothedRaw(size_t sampleOffset) const;

	const ParamRange& getRange() const { return _range; }

private:
	std::string _id = "";
	std::string _displayName = "";
	ParamRange _range;
	bool _serializable = false;
	int _controlInputIndex = -1;

	std::atomic<float> _value{ 0.0f };
	std::atomic<float> _controlScale{ 0.0f };
    std::atomic<float> _controlInput{ 0.0f };

    // for smoothing
	float _normStart = 0.0f;
	float _normEnd = 0.0f;
	float _normInc = 0.0f;
	float _scaleStart = 0.0f;
	float _scaleEnd = 0.0f;
	float _scaleInc = 0.0f;
    float _inputStart = 0.0f;
    float _inputEnd = 0.0f;
    float _inputInc = 0.0f;
};
}
