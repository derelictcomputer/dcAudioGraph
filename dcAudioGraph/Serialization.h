#pragma once

// TODO: remove this
#define DC_AUDIO_GRAPH_SERIALIZATION 1

#if DC_AUDIO_GRAPH_SERIALIZATION
#include "json.hpp"

namespace dc
{
class Graph;
class Module;

/*
 * Use this to create a module when you can't do it explicitly (such as when deserializing)
 * NOTE: If you make a new Module, you'll need to register it with the factory.
 */
class ModuleFactory final
{
public:
	using ModuleCreateMethod = std::function<std::unique_ptr<Module>()>;

	ModuleFactory() = delete;

	static bool registerModule(const std::string& name, ModuleCreateMethod moduleCreateMethod);
	static std::unique_ptr<Module> create(const std::string& name);

private:
	static std::map<std::string, ModuleCreateMethod> _moduleCreateMethods;
};

// call this to register the built-in module types
void registerBuiltInModules();

// call this to register your own module type
// NOTE: this isn't necessary unless you're doing serialization of Graph and Module instances
bool registerModule(const std::string& moduleId, ModuleFactory::ModuleCreateMethod createMethod);

// JSON serialization
void to_json(nlohmann::json& j, const Graph& g);
void from_json(const nlohmann::json& j, Graph& g);
void to_json(nlohmann::json& j, const Module& m);
void from_json(const nlohmann::json& j, Module& m);
}
#endif
