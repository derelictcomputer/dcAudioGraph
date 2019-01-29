#pragma once
#include "AudioBuffer.h"
#include <vector>
#include <memory>
#include <atomic>

namespace dc
{
struct Connection final
{
	enum class Type
	{
		Audio,
		Control,
        Event
	};

	bool operator==(const Connection& other) const;
	bool operator!=(const Connection& other) const { return !(*this == other); }

	size_t fromId;
	size_t fromIdx;
	size_t toId;
	size_t toIdx;
	Type type;
};

class EventBuffer
{
public:
	void merge(EventBuffer& other);
	void merge(EventBuffer& other, size_t fromIdx, size_t toIdx);
	void clear();
};

class Module
{
public:
	friend class Graph;

	Module() = default;
	virtual ~Module() = default;

	size_t getId() const { return 0; }

	virtual void process() {}

	size_t numAudioIn;
	size_t numAudioOut;
	size_t numControlIn;
	size_t numControlOut;
	size_t numEventIn;
	size_t numEventOut;
	double sampleRate;

protected:
	AudioBuffer _audioBuffer;
	AudioBuffer _controlBuffer;
	EventBuffer _eventBuffer;

private:

};


class Graph : public Module
{
public:
    void process() override;
    void process(AudioBuffer& audio, AudioBuffer& control, EventBuffer& events) const;

	Module* getModuleById(size_t id);

private:
	struct ModuleRenderInfo final
    {
		struct InputInfo
		{
			Module* module;
			Connection::Type type;
			size_t fromIdx;
			size_t toIdx;
		};

		Module* module = nullptr;
		std::vector<InputInfo> inputs;
	};

	struct GraphProcessContext final
    {
		std::vector<ModuleRenderInfo> modules;
	};

    static void processModule(ModuleRenderInfo& m);

	void updateProcessContext();
	ModuleRenderInfo makeModuleRenderInfo(Module& m);
	bool moduleIsInputTo(Module* from, Module* to);
	bool getInputConnectionsForModule(Module& m, std::vector<Connection>& connections);

	std::vector<Connection> _connections;
	Module _inputModule;
	Module _outputModule;
	std::vector<std::unique_ptr<Module>> _modules;
	std::shared_ptr<GraphProcessContext> _processContext;

	std::vector<std::unique_ptr<Module>> _modulesToRelease;
};
}
