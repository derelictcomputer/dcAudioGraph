/*
 * A buffer of event messages.
 */

#pragma once

#include <vector>

namespace dc
{
// A single message
// This can be a trigger or a MIDI-style note
struct EventMessage final
{
	// Specifies the type of the message.
	// Also acts as a bitfield for filtering message types in other things.
	enum Type : uint16_t
	{
		Trigger = 0x0001,
		Note	= 0x0002,
		None	= 0x0000,
		All		= 0xffff
	};

	struct NoteParams
	{
		int noteNumber = 0;
		float gain = 1.0f;
	};

	EventMessage();
	explicit EventMessage(Type type, size_t sampleOffset);

	Type type;
	size_t sampleOffset;

	union
	{
		uint8_t noParam;
		NoteParams noteParam;
	};
};

class EventBuffer final
{
public:
	class Channel final
	{
	public:
		class Iterator final
		{
		public:
			explicit Iterator(Channel& channel);
			bool next(EventMessage& messageOut);

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

		void insert(EventMessage& message);
		void merge(Channel& other);
		void clear();

	private:
		std::vector<EventMessage> _messages;
	};

	size_t getNumChannels() const { return _channels.size(); }
	void setNumChannels(size_t numChannels);

	size_t getNumMessages(size_t channelIndex);

	Channel::Iterator getIterator(size_t channelIdx);

	void insert(EventMessage& message, size_t channelIndex);
	void merge(EventBuffer& from);
	void merge(EventBuffer& from, size_t fromIndex, size_t toIndex);
	void clear();
	void clear(size_t channelIndex);

private:
	std::vector<Channel> _channels;
};
}