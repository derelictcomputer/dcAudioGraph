/*
 * A simple gain slider, which can also be used as a simple summing bus.
 */

#pragma once

#include "Module.h"

namespace dc
{
class Gain : public Module
{
public:
	Gain();

private:
    void process(ModuleProcessContext& context) override;

	static float getNormalized(float rawValue, float min, float max);
	static float getRaw(float normalizedValue, float min, float max);

	static float dbToLin(float db);

    float _lastCtlInput = 0.0f;
};
}
