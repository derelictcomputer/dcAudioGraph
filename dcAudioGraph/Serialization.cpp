#include "Serialization.h"
#include "Graph.h"
#include "Modules/GraphModule.h"

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

void dc::registerBuiltInModules()
{
#define MODULE_CREATE_FN(moduleType) [](){ return std::make_unique<moduleType>(); }

	ModuleFactory::registerModule(GraphInputModule::getModuleId(), MODULE_CREATE_FN(GraphInputModule));
	ModuleFactory::registerModule(GraphOutputModule::getModuleId(), MODULE_CREATE_FN(GraphOutputModule));
	ModuleFactory::registerModule(GraphModule::getModuleId(), MODULE_CREATE_FN(GraphModule));

#undef MODULE_CREATE_FN
}

bool dc::registerModule(const std::string& moduleId, ModuleFactory::ModuleCreateMethod createMethod)
{
	return ModuleFactory::registerModule(moduleId, createMethod);
}
