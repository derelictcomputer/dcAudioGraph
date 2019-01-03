#pragma once
#include "Module.h"

namespace dc
{
/*
 * Use this to create a module when you can't do it explicitly (such as when deserializing)
 * NOTE: If you make a new Module, you'll need to register it with the factory.
 */
class ModuleFactory
{
public:
	using ModuleCreateMethod = std::unique_ptr<Module>(*)();

	ModuleFactory() = delete;

	static bool registerModule(const std::string& name, ModuleCreateMethod moduleCreateMethod);
	static std::unique_ptr<Module> create(const std::string& name);

private:
	static std::map<std::string, ModuleCreateMethod> _moduleCreateMethods;
};
}
