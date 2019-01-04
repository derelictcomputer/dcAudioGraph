#pragma once
#include <vector>
#include <memory>

namespace dc
{
struct ControlMessage
{
public:
	enum class Type
	{
		Trigger,
		Note,
		Float
	};

	struct NoParams {};

	struct NoteParams
	{
		int noteNumber = 0;
		float gain = 1.0f;
	};

	struct FloatParams
	{
		float value = 0.0f;
	};

	ControlMessage();
	explicit ControlMessage(Type type, size_t sampleOffset);

	Type type;
	size_t sampleOffset;

	union
	{
		NoParams noParams;
		NoteParams noteParams;
		FloatParams floatParams;
	};
};

class ControlBuffer
{
public:
	class Iterator
	{
	public:
		explicit Iterator(ControlBuffer& buffer);
		bool next(ControlMessage& messageOut);

		static Iterator invalid;

	private:
		Iterator();
		static ControlBuffer _invalidBuffer;

		ControlBuffer& _buffer;
		size_t _next = 0;
	};

	explicit ControlBuffer(size_t maxSize);

	size_t size() const { return _messages.size(); };
	void insert(ControlMessage message);
	void merge(ControlBuffer& other);
	void clear();

private:
	std::vector<ControlMessage> _messages;
	size_t _maxSize;
};
}
