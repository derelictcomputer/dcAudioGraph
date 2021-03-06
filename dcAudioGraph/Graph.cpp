#include "Graph.h"
#include <algorithm>
#include <cassert>

bool dc::Connection::operator==(const Connection& other) const
{
  return fromId == other.fromId &&
         fromIdx == other.fromIdx &&
         toId == other.toId &&
         toIdx == other.toIdx &&
         type == other.type;
}

dc::Graph::Graph()
{
  _inputModule._id = 1;
  _outputModule._id = 2;
  updateGraphProcessContext();
}

void dc::Graph::process(ModuleProcessContext& context)
{
  process(context.audioBuffer, context.eventBuffer);
}

void dc::Graph::process(AudioBuffer& audio, EventBuffer& events) const
{
  // get the context
  auto context = std::atomic_load(&_graphProcessContext);

  // this could be valid, so handle it
  if (nullptr == context || context->modules.empty())
  {
    return;
  }

  // copy input to input module
  if (auto* input = context->modules[0].module)
  {
    auto mCtx = std::atomic_load(&input->_processContext);

    // clear in case there are different numbers of channels
    mCtx->audioBuffer.zero();
    mCtx->eventBuffer.clear();

    mCtx->audioBuffer.copyFrom(audio, false);
    mCtx->eventBuffer.merge(events);
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
    auto mCtx = std::atomic_load(&output->_processContext);

    // clear in case there are different numbers of channels
    audio.zero();
    events.clear();

    audio.copyFrom(mCtx->audioBuffer, false);
    events.merge(mCtx->eventBuffer);
  }
  else
  {
    assert(false);
  }
}

void dc::Graph::processModule(ModuleRenderInfo& m)
{
  assert(nullptr != m.module);

  auto ctx = std::atomic_load(&m.module->_processContext);

  if (nullptr == ctx)
  {
    return;
  }

  // if this module has inputs, pull in the input data
  if (ctx->numAudioIn > 0 || ctx->numEventIn > 0)
  {
    ctx->audioBuffer.zero();
    ctx->eventBuffer.clear();

    for (auto inputInfo : m.inputs)
    {
      auto inCtx = std::atomic_load(&inputInfo.module->_processContext);

      if (nullptr == inCtx)
      {
        continue;
      }

      switch (inputInfo.type)
      {
        case Connection::Type::Audio:
          ctx->audioBuffer.addFrom(inCtx->audioBuffer, inputInfo.fromIdx, inputInfo.toIdx);
          break;
        case Connection::Type::Event:
        {
          auto it = inCtx->eventBuffer.getIterator(inputInfo.fromIdx);
          EventMessage msg;
          while (it.next(msg))
          {
            if (eventMessageTypeMatches(inputInfo.eventTypeFlags, msg.type))
            {
              ctx->eventBuffer.insert(msg, inputInfo.toIdx);
            }
          }
        }
          break;
        default:;
      }
    }
  }

  m.module->process(*ctx);
}

void dc::Graph::updateGraphProcessContext()
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
  newContext = std::atomic_exchange(&_graphProcessContext, newContext);
  // spin here in case process() is still using the old context
  while (newContext.use_count() > 1)
  {}

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
        EventMessage::Type emType = EventMessage::None;
        if (c.type == Connection::Type::Event)
        {
          if (auto* io = m.getIo(Event | Input, c.toIdx))
          {
            emType = io->eventTypeFlags;
          }
        }

        info.inputs.push_back({upstream, c.type, c.fromIdx, c.toIdx, emType});
      }
    }
  }

  return info;
}

void dc::Graph::clear()
{
  while (!_modules.empty())
  {
    removeModuleAt(0);
  }
  _nextModuleId = 3;
}

size_t dc::Graph::addModule(std::unique_ptr<Module> module, size_t graphId)
{
  if (nullptr == module)
  {
    return 0;
  }

  size_t id = 0;
  if (graphId > 0)
  {
    if (auto* m = getModuleById(graphId))
    {
      return false;
    }
    id = graphId;
    if (id >= _nextModuleId)
    {
      _nextModuleId = id + 1;
    }
  }
  else
  {
    id = _nextModuleId++;
  }

  module->_id = id;

  module->setBlockSize(_blockSize);
  module->setSampleRate(_sampleRate);

  _modules.push_back(std::move(module));

  updateGraphProcessContext();

  return id;
}

dc::Module* dc::Graph::getModuleAt(size_t index)
{
  if (index < _modules.size())
  {
    return _modules[index].get();
  }
  return nullptr;
}

dc::Module* dc::Graph::getModuleById(size_t id)
{
  if (id == _inputModule.getId())
  {
    return &_inputModule;
  }

  if (id == _outputModule.getId())
  {
    return &_outputModule;
  }

  for (auto& m : _modules)
  {
    if (m->getId() == id)
    {
      return m.get();
    }
  }
  return nullptr;
}

bool dc::Graph::removeModuleAt(size_t index)
{
  return removeModuleInternal(index);
}

bool dc::Graph::removeModuleById(size_t id)
{
  for (size_t i = 0; i < _modules.size(); ++i)
  {
    if (_modules[i]->getId() == id)
    {
      return removeModuleAt(i);
    }
  }
  return false;
}

bool dc::Graph::addConnection(const Connection& connection)
{
  if (!connectionIsValid(connection))
  {
    return false;
  }

  _allConnections.push_back(connection);

  updateGraphProcessContext();

  return true;
}

void dc::Graph::removeConnection(const Connection& connection)
{
  if (!connectionExists(connection))
  {
    return;
  }

  for (size_t i = 0; i < _allConnections.size(); ++i)
  {
    // handle zeroing the control input if there is one
    if (_allConnections[i] == connection)
    {
      _allConnections.erase(_allConnections.begin() + i);

      if (connection.type == Connection::Type::Event)
      {
        if (auto* m = getModuleById(connection.toId))
        {
          bool hasOtherConnection = false;
          std::vector<Connection> cs;
          if (getInputConnectionsForModule(*m, cs))
          {
            for (auto& c : cs)
            {
              if (c.toIdx == connection.toIdx)
              {
                hasOtherConnection = true;
                break;
              }
            }
          }

          if (!hasOtherConnection)
          {
            for (auto& p : m->_params)
            {
              if (p->getControlInputIndex() == static_cast<int>(connection.toIdx))
              {
                p->setControlInput(0.0f);
                break;
              }
            }
          }
        }
      }

      updateGraphProcessContext();

      break;
    }
  }
}

bool dc::Graph::getConnection(size_t index, Connection& connectionOut)
{
  if (index < _allConnections.size())
  {
    connectionOut = _allConnections[index];
    return true;
  }
  return false;
}

void dc::Graph::disconnectModule(size_t id)
{
  if (auto* m = getModuleById(id))
  {
    size_t i = 0;
    while (i < _allConnections.size())
    {
      auto& c = _allConnections[i];
      if (c.fromId == id || c.toId == id)
      {
        removeConnection(c);
      }
      else
      {
        ++i;
      }
    }
  }
}

void dc::Graph::blockSizeChanged()
{
  _inputModule.setBlockSize(_blockSize);
  _outputModule.setBlockSize(_blockSize);
  for (auto& m : _modules)
  {
    m->setBlockSize(_blockSize);
  }
}

bool dc::Graph::addIoInternal(std::vector<Io>& io, const std::string& description, EventMessage::Type controlType)
{
  if (!Module::addIoInternal(io, description, controlType))
  {
    return false;
  }

  if (&io == &_audioInputs)
  {
    return _inputModule.addIo(Audio | Output, description, controlType);
  }
  if (&io == &_audioOutputs)
  {
    return _outputModule.addIo(Audio | Input, description, controlType);
  }
  if (&io == &_eventInputs)
  {
    return _inputModule.addIo(Event | Output, description, controlType);
  }
  if (&io == &_eventOutputs)
  {
    return _outputModule.addIo(Event | Input, description, controlType);
  }

  return false;
}

bool dc::Graph::removeIoInternal(std::vector<Io>& io, size_t index)
{
  if (!Module::removeIoInternal(io, index))
  {
    return false;
  }

  if (&io == &_audioInputs)
  {
    return _inputModule.removeIo(Audio | Output, index);
  }
  if (&io == &_audioOutputs)
  {
    return _outputModule.removeIo(Audio | Input, index);
  }
  if (&io == &_eventInputs)
  {
    return _inputModule.removeIo(Event | Output, index);
  }
  if (&io == &_eventOutputs)
  {
    return _outputModule.removeIo(Event | Input, index);
  }

  return false;
}

bool dc::Graph::connectionIsValid(const Connection& connection)
{
  if (connectionExists(connection))
  {
    return false;
  }

  Module* from = nullptr;
  Module* to = nullptr;

  if (!getModulesForConnection(connection, from, to))
  {
    return false;
  }

  switch (connection.type)
  {
    case Connection::Type::Audio:
    {
      if (connection.fromIdx >= from->getNumIo(Audio | Output) || connection.toIdx >= to->getNumIo(Audio | Input))
      {
        return false;
      }
      break;
    }
    case Connection::Type::Event:
    {
      const auto fromFlags = from->getEventIoFlags(connection.fromIdx, false);
      const auto toFlags = to->getEventIoFlags(connection.toIdx, true);
      if ((fromFlags & toFlags) == 0)
      {
        return false;
      }
      break;
    }
    default:
      return false;
  }

  return !connectionCreatesLoop(connection);
}

bool dc::Graph::connectionExists(const Connection& connection)
{
  for (auto& c : _allConnections)
  {
    if (c == connection)
    {
      return true;
    }
  }
  return false;
}

bool dc::Graph::getModulesForConnection(const Connection& connection, Module*& from, Module*& to)
{
  from = getModuleById(connection.fromId);
  to = getModuleById(connection.toId);
  return nullptr != from && nullptr != to;
}

bool dc::Graph::connectionCreatesLoop(const Connection& connection)
{
  Module* from = nullptr;
  Module* to = nullptr;

  if (!getModulesForConnection(connection, from, to))
  {
    return false;
  }

  return moduleIsInputTo(to, from);
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
  for (auto& c : _allConnections)
  {
    if (c.toId == m.getId())
    {
      connections.push_back(c);
    }
  }
  return !connections.empty();
}

bool dc::Graph::removeModuleInternal(size_t index)
{
  if (index >= _modules.size())
  {
    return false;
  }

  // disconnect the module
  disconnectModule(_modules[index]->_id);

  // stick the module into the release pool
  _modulesToRelease.emplace_back(_modules[index].release());
  _modules.erase(_modules.begin() + index);

  // update the process context
  updateGraphProcessContext();

  return true;
}
