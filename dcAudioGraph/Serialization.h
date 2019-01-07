#pragma once
#include <map>
#include <memory>
#include <functional>

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
bool registerModule(const std::string& moduleId, ModuleFactory::ModuleCreateMethod createMethod);
}
