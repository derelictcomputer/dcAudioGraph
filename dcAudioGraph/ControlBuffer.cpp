#include "ControlBuffer.h"

dc::ControlMessage::ControlMessage() : type(Type::Trigger), sampleOffset(0)
{
	noParam = 0;
}

dc::ControlMessage::ControlMessage(Type type, size_t sampleOffset) : sampleOffset(sampleOffset)
{
	// ensure a valid type is used
	switch (type)
	{
	case Type::Note:
		this->type = type;
		noteParam = { 0, 1.0f };
		break;
	case Type::Float:
		this->type = type;
		floatParam = 0.0f;
		break;
	case Type::Trigger:
	default:
		this->type = Type::Trigger;
		noParam = 0;
		break;
	}
}

dc::ControlBuffer::Channel::Iterator dc::ControlBuffer::Channel::Iterator::invalid;
dc::ControlBuffer::Channel dc::ControlBuffer::Channel::Iterator::_invalidChannel;

dc::ControlBuffer::Channel::Iterator::Iterator(Channel& channel) : _channel(channel)
{
}

bool dc::ControlBuffer::Channel::Iterator::next(ControlMessage& messageOut)
{
	if (_channel._messages.empty() || _next >= _channel._messages.size())
	{
		return false;
	}
	messageOut = _channel._messages[_next++];
	return true;
}

dc::ControlBuffer::Channel::Iterator::Iterator() : _channel(_invalidChannel)
{
}

dc::ControlBuffer::Channel::Channel()
{
	// try to avoid re-allocations in the process chain
	// TODO: this should be configurable somewhere
	_messages.reserve(1024);
}

void dc::ControlBuffer::Channel::insert(ControlMessage& message)
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

void dc::ControlBuffer::Channel::merge(Channel& other)
{
	Iterator otherIt(other);
	ControlMessage msg;
	while (otherIt.next(msg))
	{
		insert(msg);
	}
}

void dc::ControlBuffer::Channel::clear()
{
	_messages.clear();
}

void dc::ControlBuffer::setNumChannels(size_t numChannels)
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

dc::ControlBuffer::Channel::Iterator dc::ControlBuffer::getIterator(size_t channelIdx)
{
	if (channelIdx < _channels.size())
	{
		return _channels[channelIdx].getIterator();
	}
	return Channel::Iterator::invalid;
}

void dc::ControlBuffer::insert(ControlMessage& message, size_t channelIndex)
{
	if (channelIndex < _channels.size())
	{
		_channels[channelIndex].insert(message);
	}
}

void dc::ControlBuffer::merge(ControlBuffer& from)
{
	for (size_t i = 0; i < _channels.size(); ++i)
	{
		if (i < from.getNumChannels())
		{
			_channels[i].merge(from._channels[i]);
		}
	}
}

void dc::ControlBuffer::merge(ControlBuffer& from, size_t fromIndex, size_t toIndex)
{
	if (fromIndex < from.getNumChannels() && toIndex < getNumChannels())
	{
		_channels[toIndex].merge(from._channels[fromIndex]);
	}
}

void dc::ControlBuffer::clear()
{
	for (auto& channel : _channels)
	{
		channel.clear();
	}
}

void dc::ControlBuffer::clear(size_t channelIndex)
{
	if (channelIndex < _channels.size())
	{
		_channels[channelIndex].clear();
	}
}
