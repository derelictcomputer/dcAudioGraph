#pragma once

// This is the simplest way to use dcAudioGraph. Just include this and go to town.
// But if you're not doing serialization, there's no reason you can't just include the headers individually.

// Include core headers
#include "AudioBuffer.h"
#include "Module.h"
#include "Graph.h"

// Include headers for built-in modules
#include "Modules/GraphModule.h"

namespace dc
{

// call this to register the built-in module types
void registerBuiltInModules();

// call this to register your own module type
// NOTE: this isn't necessary unless you're doing serialization of Graph and Module instances
bool registerModule(const std::string& moduleId, ModuleFactory::ModuleCreateMethod createMethod);

}
