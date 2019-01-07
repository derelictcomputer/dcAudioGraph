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
	Graph& getGraph() { return _graph; }

	std::string getName() override { return "Graph Module"; }

protected:
	void onProcess() override;
	void onRefreshAudioBuffers() override;

private:
	Graph _graph;
};
}
