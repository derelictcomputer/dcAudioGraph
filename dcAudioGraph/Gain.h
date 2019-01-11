#pragma once

#include "Module.h"

namespace dc
{
class Gain : public Module
{
public:
	Gain();

	std::string getName() override { return "Gain"; }

protected:
	void onProcess() override;

};
}
