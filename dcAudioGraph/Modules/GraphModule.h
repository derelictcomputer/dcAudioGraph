/*
  ==============================================================================

    GraphModule.h
    Created: 27 Dec 2018 7:34:18pm
    Author:  Charlie Huguenard

  ==============================================================================
*/

#pragma once

#include "../Module.h"
#include "../Graph.h"

namespace dc
{
class GraphModule : public Module
{
public:
	GraphModule() = default;
	~GraphModule() override = default;

	Graph& getGraph() { return _graph; }

protected:
	void onProcess() override;
	void onRefreshBuffers() override;

	json toJsonInternal() const override;
	void fromJsonInternal(const json& j) override;
	std::string getModuleIdForInstance() const override { return getModuleId(); }

private:
	Graph _graph;

	// register with the ModuleFactory so we can deserialize
	static std::unique_ptr<Module> createMethod() { return std::make_unique<GraphModule>(); }
	static std::string getModuleId() { return "dc.GraphModule"; }
	static bool _registered;
};
}
