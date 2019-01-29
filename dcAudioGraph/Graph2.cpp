#include "Graph2.h"
#include <cassert>

void dc::Graph::process()
{
	process(_audioBuffer, _controlBuffer, _eventBuffer);
}

void dc::Graph::process(AudioBuffer& audio, AudioBuffer& control, EventBuffer& events) const
{
	// get the context
	auto context = std::atomic_load(&_processContext);

    // this could be valid, so handle it
    if (context->modules.empty())
    {
		return;
    }

    // copy input to input module
    if (auto* input = context->modules[0].module)
    {
		input->_audioBuffer.copyFrom(audio, false);
		input->_controlBuffer.copyFrom(control, false);
		input->_eventBuffer.clear();
		input->_eventBuffer.merge(events);
    }
    else
    {
		assert(false);
		return;
    }

	// process the modules
	for (auto& m : context->modules)
	{
		processModule(m);
	}

    // copy output from output module
    if (auto* output = context->modules[context->modules.size() - 1].module)
    {
		audio.copyFrom(output->_audioBuffer, false);
		control.copyFrom(output->_controlBuffer, false);
		events.clear();
		events.merge(output->_eventBuffer);
    }
    else
    {
		assert(false);
    }
}

void dc::Graph::processModule(ModuleRenderInfo& m)
{
	assert(nullptr != m.module);

    // if this module has inputs, pull in the input data
    if (!m.inputs.empty())
    {
		m.module->_audioBuffer.zero();
		m.module->_controlBuffer.zero();
		m.module->_eventBuffer.clear();

        for (auto inputInfo : m.inputs)
        {
            switch (inputInfo.type)
            {
            case Connection::Type::Audio: 
				m.module->_audioBuffer.addFrom(inputInfo.module->_audioBuffer, inputInfo.fromIdx, inputInfo.toIdx);
                break;
            case Connection::Type::Control: 
				m.module->_controlBuffer.addFrom(inputInfo.module->_controlBuffer, inputInfo.fromIdx, inputInfo.toIdx);
                break;
            case Connection::Type::Event: 
				m.module->_eventBuffer.merge(inputInfo.module->_eventBuffer, inputInfo.fromIdx, inputInfo.toIdx);
                break;
            default: ;
            }
        }
    }

	m.module->process();
}

void dc::Graph::updateProcessContext()
{
	auto newContext = std::make_shared<GraphProcessContext>();

    // build the context
	newContext->modules.push_back(makeModuleRenderInfo(_inputModule));
    for (auto& m : _modules)
    {
		size_t insertIdx = 0;
        for (; insertIdx < newContext->modules.size(); ++insertIdx)
        {
            if (moduleIsInputTo(m.get(), newContext->modules[insertIdx].module))
            {
                break;
            }
        }
		auto renderInfo = makeModuleRenderInfo(*m);
		newContext->modules.insert(newContext->modules.begin() + insertIdx, renderInfo);
    }
	newContext->modules.push_back(makeModuleRenderInfo(_outputModule));

    // swap in the new context
	newContext = std::atomic_exchange(&_processContext, newContext);
    // spin here in case process() is still using the old context
	while (newContext.use_count() > 1){}

    // now that the old context is gone, we can clear the released modules
	_modulesToRelease.clear();
}

dc::Graph::ModuleRenderInfo dc::Graph::makeModuleRenderInfo(Module& m)
{
	ModuleRenderInfo info;
	info.module = &m;

	std::vector<Connection> connections;
	if (getInputConnectionsForModule(m, connections))
	{
		for (auto& c : connections)
		{
			if (auto* upstream = getModuleById(c.fromId))
			{
				info.inputs.push_back({ upstream, c.type, c.fromIdx, c.toIdx });
			}
		}
	}

	return info;
}

bool dc::Graph::moduleIsInputTo(Module* from, Module* to)
{
	if (from == to)
	{
		return true;
	}

	if (nullptr == from || nullptr == to)
	{
		return false;
	}

	std::vector<Connection> connections;
	if (getInputConnectionsForModule(*to, connections))
	{
		for (auto& c : connections)
		{
			if (auto* m = getModuleById(c.fromId))
			{
				if (moduleIsInputTo(from, m))
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool dc::Graph::getInputConnectionsForModule(Module& m, std::vector<Connection>& connections)
{
	for (auto& c : _connections)
	{
		if (c.toId == m.getId())
		{
			connections.push_back(c);
		}
	}
	return !connections.empty();
}
