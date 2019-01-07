#pragma once
#include <vector>

namespace dc
{
struct ControlMessage final
{
	// Specifies the type of the message.
	// Also acts as a bitfield for filtering message types in other things.
	enum Type : uint16_t
	{
		Trigger = 0x0001,
		Note	= 0x0002,
		Float	= 0x0004,
		None	= 0x0000,
		All		= 0xffff
	};

	using NoParams = uint8_t;

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

class ControlBuffer final
{
public:
	class Channel final
	{
	public:
		class Iterator final
		{
		public:
			explicit Iterator(Channel& channel);
			bool next(ControlMessage& messageOut);

			static Iterator invalid;

		private:
			Iterator();

			static Channel _invalidChannel;

			Channel& _channel;
			size_t _next = 0;
		};

		explicit Channel();

		size_t size() const { return _messages.size(); };
		Iterator getIterator() { return Iterator(*this); }

		void insert(ControlMessage& message);
		void merge(Channel& other);
		void clear();

	private:
		std::vector<ControlMessage> _messages;
	};

	size_t getNumChannels() const { return _channels.size(); }
	void setNumChannels(size_t numChannels);

	Channel::Iterator getIterator(size_t channelIdx);

	void insert(ControlMessage& message, size_t channelIndex);
	void merge(ControlBuffer& from);
	void merge(ControlBuffer& from, size_t fromIndex, size_t toIndex);
	void clear();
	void clear(size_t channelIndex);

private:
	std::vector<Channel> _channels;
};
}