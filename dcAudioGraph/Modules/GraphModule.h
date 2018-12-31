/*
  ==============================================================================

    GraphModule.h
    Created: 27 Dec 2018 7:34:18pm
    Author:  Charlie Huguenard

  ==============================================================================
*/

#pragma once

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

private:
	Graph _graph;
};
}
