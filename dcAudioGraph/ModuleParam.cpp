#include "ModuleParam.h"
#include <algorithm>
#include <utility>

dc::ParamRange::ParamRange(float min, float max, float stepSize) :
	_min(min),
	_max(max)
{
	_stepSize = std::max(0.0f, stepSize);
}

dc::ParamRange::ParamRange(float min, float max, float stepSize, GetNormFn getNormalized, GetRawFn getRaw) :
	ParamRange(min, max, stepSize)
{
	if (nullptr != getNormalized)
	{
		_getNormalized = getNormalized;
	}
	if (nullptr != getRaw)
	{
		_getRaw = getRaw;
	}
}

dc::ParamRange& dc::ParamRange::operator=(const ParamRange& other)
{
	if (this != &other)
	{
		_min = other._min;
		_max = other._max;
		_stepSize = other._stepSize;
		_getNormalized = other._getNormalized;
		_getRaw = other._getRaw;
	}
	return *this;
}

dc::ParamRange::ParamRange(ParamRange&& other) noexcept :
	_min(other._min),
	_max(other._max),
	_stepSize(other._stepSize),
	_getNormalized(other._getNormalized),
	_getRaw(other._getRaw)
{
}

dc::ParamRange& dc::ParamRange::operator=(ParamRange&& other) noexcept
{
	if (this != &other)
	{
		_min = other._min;
		_max = other._max;
		_stepSize = other._stepSize;
		_getNormalized = other._getNormalized;
		_getRaw = other._getRaw;
	}
	return *this;
}

float dc::ParamRange::getNormalized(float rawValue) const
{
	const float constrained = constrainRaw(rawValue);
	const float normalized = _getNormalized(constrained, _min, _max);
	return normalized;
}

float dc::ParamRange::getRaw(float normalizedValue) const
{
	const float raw = _getRaw(normalizedValue, _min, _max);
	const float constrained = constrainRaw(raw);
	return constrained;
}

float dc::ParamRange::constrainRaw(float rawValue) const
{
	const float clamped = clampToRange(rawValue, _min, _max);
	const float snapped = snapToStep(clamped, _stepSize);
	return snapped;
}

float dc::ParamRange::clampToRange(float value, float min, float max)
{
	if (max > min)
	{
		return std::min(max, std::max(min, value));
	}
	else
	{
		return std::max(max, std::min(min, value));
	}
}

float dc::ParamRange::snapToStep(float value, float stepSize)
{
	if (stepSize > 0.0f)
	{
		return stepSize * std::floor(value / stepSize + 0.5f);
	}
	return value;
}

float dc::ParamRange::getNormalizedLinear(float rawValue, float min, float max)
{
	return (rawValue - min) / (max - min);
}

float dc::ParamRange::getRawLinear(float normalizedValue, float min, float max)
{
	return min + (max - min) * normalizedValue;
}

dc::ModuleParam::ModuleParam(std::string id, std::string displayName, const ParamRange& range,
	bool serializable, int controlInputIndex) :
	_id(std::move(id)),
	_displayName(std::move(displayName)),
	_range(range),
	_serializable(serializable),
	_controlInputIndex(controlInputIndex)
{
}

float dc::ModuleParam::getNormalized() const
{
	return _range.getNormalized(_value);
}

void dc::ModuleParam::setNormalized(float normalizedValue)
{
	_value = _range.getRaw(normalizedValue);
}

void dc::ModuleParam::setRaw(float rawValue)
{
	_value = _range.constrainRaw(rawValue);
}
