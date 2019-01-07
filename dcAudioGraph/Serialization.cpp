#include "Serialization.h"
#include "Graph.h"
#include "Modules/GraphModule.h"

#if DC_AUDIO_GRAPH_SERIALIZATION

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
#define moduleCreateFn(moduleType) []{ return std::make_unique<moduleType>(); }

	ModuleFactory::registerModule(GraphInputModule::getModuleId(), moduleCreateFn(GraphInputModule));
	ModuleFactory::registerModule(GraphOutputModule::getModuleId(), moduleCreateFn(GraphOutputModule));
	ModuleFactory::registerModule(GraphModule::getModuleId(), moduleCreateFn(GraphModule));

#undef moduleCreateFn
}

bool dc::registerModule(const std::string& moduleId, ModuleFactory::ModuleCreateMethod createMethod)
{
	return ModuleFactory::registerModule(moduleId, createMethod);
}

using json = nlohmann::json;

void dc::to_json(json& j, const Graph& g)
{
}

void dc::from_json(const json& j, Graph& g)
{
}

void dc::to_json(json& j, const Module& m)
{
}

void dc::from_json(const json& j, Module& m)
{
}

#endif

/*
// serialization keys
const std::string S_GRAPH = "graph";

json dc::GraphModule::toJsonInternal() const
{
	json j;
	j[S_GRAPH] = _graph.toJson();
	return j;
}

void dc::GraphModule::fromJsonInternal(const json& j)
{
	_graph.fromJson(j[S_GRAPH]);
}
 */
/*
 // serialization keys
const std::string S_AUDIO_INPUT_MODULE = "audioInputModule";
const std::string S_AUDIO_OUTPUT_MODULE = "audioOutputModule";
const std::string S_MODULES = "modules";

json dc::Graph::toJson() const
{
	json j;

	j[S_AUDIO_INPUT_MODULE] = _inputModule.toJson();
	j[S_AUDIO_OUTPUT_MODULE] = _outputModule.toJson();
	
	auto m = json::array();
	for (auto& module : _modules)
	{
		m.push_back(module->toJson());
	}
	j[S_MODULES] = m;

	return j;
}

void dc::Graph::fromJson(const json& j)
{
	clear();

	_inputModule.fromJson(j[S_AUDIO_INPUT_MODULE]);
	_outputModule.fromJson(j[S_AUDIO_OUTPUT_MODULE]);

	// first, create all the modules
	auto modules = j[S_MODULES];
	for (auto& module : modules)
	{
		auto instance = Module::createFromJson(module);
		if (nullptr != instance)
		{
			addModule(std::move(instance), instance->id);
		}
	}
	// now, connect them
	for (auto& module : modules)
	{
		Module::updateConnectionsFromJson(module, *this);
	}
	// also connect the input and output modules
	Module::updateConnectionsFromJson(j[S_AUDIO_INPUT_MODULE], *this);
	Module::updateConnectionsFromJson(j[S_AUDIO_OUTPUT_MODULE], *this);

	compressIds();
}
 */

/*

// serialization keys
const std::string S_MODULE_ID = "_moduleId";
const std::string S_GRAPH_ID = "_graphId";
const std::string S_NUM_AUDIO_INPUTS = "numAudioInputs";
const std::string S_NUM_AUDIO_OUTPUTS = "numAudioOutputs";
const std::string S_AUDIO_INPUT_CONNECTIONS = "audioInputConnections";
const std::string S_AIC_INPUT_INDEX = "inputIndex";
const std::string S_AIC_OUTPUT_INDEX = "outputIndex";
const std::string S_AIC_OUTPUT_GRAPH_ID = "outputGraphId";
const std::string S_NUM_CONTROL_INPUTS = "numControlInputs";
const std::string S_NUM_CONTROL_OUTPUTS = "numControlOutputs";
const std::string S_CONTROL_INPUT_CONNECTIONS = "controlInputConnections";
const std::string S_CIC_INPUT_INDEX = "inputIndex";
const std::string S_CIC_OUTPUT_INDEX = "outputIndex";
const std::string S_CIC_OUTPUT_GRAPH_ID = "outputGraphId";
const std::string S_SETTINGS = "settings";

json dc::Module::toJson() const
{
	json j;
	j[S_MODULE_ID] = getModuleIdForInstance();
	j[S_GRAPH_ID] = _graphId;

	// audio I/O
	{
		j[S_NUM_AUDIO_INPUTS] = getNumAudioInputs();
		j[S_NUM_AUDIO_OUTPUTS] = getNumAudioOutputs();

		// audio input connections
		auto ic = json::array();
		for (size_t i = 0; i < getNumAudioInputs(); ++i)
		{
			for (auto& output : _audioInputs[i]->connections)
			{
				if (auto oP = output.lock())
				{
					ic.push_back({
						{S_AIC_INPUT_INDEX, i},
						{S_AIC_OUTPUT_GRAPH_ID, oP->parent._graphId},
						{S_AIC_OUTPUT_INDEX, oP->index}
						});
				}
			}
		}
		j[S_AUDIO_INPUT_CONNECTIONS] = ic;
	}

	// control I/O
	{
		j[S_NUM_CONTROL_INPUTS] = getNumControlInputs();
		j[S_NUM_CONTROL_OUTPUTS] = getNumControlOutputs();

		// control input connections
		auto ic = json::array();
		for (size_t i = 0; i < getNumControlInputs(); ++i)
		{
			for (auto& output : _audioInputs[i]->connections)
			{
				if (auto oP = output.lock())
				{
					ic.push_back({
						{S_CIC_INPUT_INDEX, i},
						{S_CIC_OUTPUT_GRAPH_ID, oP->parent._graphId},
						{S_CIC_OUTPUT_INDEX, oP->index}
					});
				}
			}
		}
		j[S_CONTROL_INPUT_CONNECTIONS] = ic;
	}

	// settings for concrete modules
	j[S_SETTINGS] = toJsonInternal();

	return j;
}

std::unique_ptr<dc::Module> dc::Module::createFromJson(const json& j)
{
	const std::string moduleId = j[S_MODULE_ID].get<std::string>();
	auto instance = ModuleFactory::create(moduleId);
	if (nullptr != instance)
	{
		instance->fromJson(j);
	}
	return instance;
}

void dc::Module::fromJson(const json& j)
{
	// do the common module stuff first, in case the specific config depends on that
	_graphId = j[S_GRAPH_ID].get<size_t>();

	// audio I/O
	setNumAudioInputs(j[S_NUM_AUDIO_INPUTS].get<size_t>());
	setNumAudioOutputs(j[S_NUM_AUDIO_OUTPUTS].get<size_t>());

	// control I/O
	setNumControlInputs(j[S_NUM_CONTROL_INPUTS].get<size_t>());
	setNumControlOutputs(j[S_NUM_CONTROL_OUTPUTS].get<size_t>());

	// NOTE: we will make the connections after all nodes have been configured for the parent graph

	fromJsonInternal(j[S_SETTINGS]);
}

void dc::Module::updateConnectionsFromJson(const json& j, Graph& parentGraph)
{
	const size_t sourceId = j[S_GRAPH_ID].get<size_t>();

	if (auto* source = parentGraph.getModuleById(sourceId))
	{
		// audio connections
		{
			auto connections = j[S_AUDIO_INPUT_CONNECTIONS];

			for (auto c : connections)
			{
				const size_t inputIdx = c[S_AIC_INPUT_INDEX].get<size_t>();
				const size_t outputModuleId = c[S_AIC_OUTPUT_GRAPH_ID].get<size_t>();
				const size_t outputIdx = c[S_AIC_OUTPUT_INDEX].get<size_t>();
				if (auto* other = parentGraph.getModuleById(outputModuleId))
				{
					connectAudio(other, outputIdx, source, inputIdx);
				}
			}
		}
		// control connections
		{
			auto connections = j[S_CONTROL_INPUT_CONNECTIONS];

			for (auto c : connections)
			{
				const size_t inputIdx = c[S_CIC_INPUT_INDEX].get<size_t>();
				const size_t outputModuleId = c[S_CIC_OUTPUT_GRAPH_ID].get<size_t>();
				const size_t outputIdx = c[S_CIC_OUTPUT_INDEX].get<size_t>();
				if (auto* other = parentGraph.getModuleById(outputModuleId))
				{
					connectControl(other, outputIdx, source, inputIdx);
				}
			}
		}
	}
}
 */