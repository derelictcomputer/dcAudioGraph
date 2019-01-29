#include "EventBuffer.h"

dc::Event::Event() : type(Type::Trigger), sampleOffset(0)
{
	noParam = 0;
}

dc::Event::Event(Type type, size_t sampleOffset) : sampleOffset(sampleOffset)
{
	// ensure a valid type is used
	switch (type)
	{
	case Type::Note:
		this->type = type;
		noteParam = { 0, 1.0f };
		break;
	case Type::Trigger:
	default:
		this->type = Type::Trigger;
		noParam = 0;
		break;
	}
}

dc::EventBuffer::Channel::Iterator dc::EventBuffer::Channel::Iterator::invalid;
dc::EventBuffer::Channel dc::EventBuffer::Channel::Iterator::_invalidChannel;

dc::EventBuffer::Channel::Iterator::Iterator(Channel& channel) : _channel(channel)
{
}

bool dc::EventBuffer::Channel::Iterator::next(Event& messageOut)
{
	if (_channel._messages.empty() || _next >= _channel._messages.size())
	{
		return false;
	}
	messageOut = _channel._messages[_next++];
	return true;
}

dc::EventBuffer::Channel::Iterator::Iterator() : _channel(_invalidChannel)
{
}

dc::EventBuffer::Channel::Channel()
{
	// try to avoid re-allocations in the process chain
	// TODO: this should be configurable somewhere
	_messages.reserve(1024);
}

void dc::EventBuffer::Channel::insert(Event& message)
{
	// keep the buffer in order of sample offset
	// TODO: faster insert, in case this gets used for large numbers of messages
	for (auto it = _messages.begin(); it != _messages.end(); ++it)
	{
		if (message.sampleOffset <= it->sampleOffset)
		{
			_messages.insert(it, message);
			return;
		}
	}
	// the message list was empty, or this should be the last element
	_messages.push_back(message);
}

void dc::EventBuffer::Channel::merge(Channel& other)
{
	Iterator otherIt(other);
	Event msg;
	while (otherIt.next(msg))
	{
		insert(msg);
	}
}

void dc::EventBuffer::Channel::clear()
{
	_messages.clear();
}

void dc::EventBuffer::setNumChannels(size_t numChannels)
{
	while (numChannels < _channels.size())
	{
		_channels.pop_back();
	}
	while (numChannels > _channels.size())
	{
		_channels.emplace_back();
	}
}

size_t dc::EventBuffer::getNumMessages(size_t channelIndex)
{
    if (channelIndex < _channels.size())
    {
		return _channels[channelIndex].size();
    }
	return 0;
}

dc::EventBuffer::Channel::Iterator dc::EventBuffer::getIterator(size_t channelIdx)
{
	if (channelIdx < _channels.size())
	{
		return _channels[channelIdx].getIterator();
	}
	return Channel::Iterator::invalid;
}

void dc::EventBuffer::insert(Event& message, size_t channelIndex)
{
	if (channelIndex < _channels.size())
	{
		_channels[channelIndex].insert(message);
	}
}

void dc::EventBuffer::merge(EventBuffer& from)
{
	for (size_t i = 0; i < _channels.size(); ++i)
	{
		if (i < from.getNumChannels())
		{
			_channels[i].merge(from._channels[i]);
		}
	}
}

void dc::EventBuffer::merge(EventBuffer& from, size_t fromIndex, size_t toIndex)
{
	if (fromIndex < from.getNumChannels() && toIndex < getNumChannels())
	{
		_channels[toIndex].merge(from._channels[fromIndex]);
	}
}

void dc::EventBuffer::clear()
{
	for (auto& channel : _channels)
	{
		channel.clear();
	}
}

void dc::EventBuffer::clear(size_t channelIndex)
{
	if (channelIndex < _channels.size())
	{
		_channels[channelIndex].clear();
	}
}
