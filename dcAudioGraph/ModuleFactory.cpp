#include "ModuleFactory.h"

std::map<std::string, dc::ModuleFactory::ModuleCreateMethod> dc::ModuleFactory::_moduleCreateMethods;

bool dc::ModuleFactory::registerModule(const std::string& name, ModuleCreateMethod moduleCreateMethod)
{
	const auto it = _moduleCreateMethods.find(name);
	if (it == _moduleCreateMethods.end())
	{
		_moduleCreateMethods[name] = moduleCreateMethod;
		return true;
	}
	return false;
}

std::unique_ptr<dc::Module> dc::ModuleFactory::create(const std::string& name)
{
	const auto it = _moduleCreateMethods.find(name);
	if (it == _moduleCreateMethods.end())
	{
		return nullptr;
	}
	return it->second();
}
