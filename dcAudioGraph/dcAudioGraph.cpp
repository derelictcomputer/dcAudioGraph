#include "dcAudioGraph.h"

void dc::registerBuiltInModules()
{
#define moduleCreateFn(moduleType) []{ return std::make_unique<moduleType>(); }

	ModuleFactory::registerModule(GraphAudioInputModule::getModuleId(), moduleCreateFn(GraphAudioInputModule));
	ModuleFactory::registerModule(GraphAudioOutputModule::getModuleId(), moduleCreateFn(GraphAudioOutputModule));
	ModuleFactory::registerModule(GraphModule::getModuleId(), moduleCreateFn(GraphModule));

#undef moduleCreateFn
}

bool dc::registerModule(const std::string& moduleId, ModuleFactory::ModuleCreateMethod createMethod)
{
	return ModuleFactory::registerModule(moduleId, createMethod);
}
