#include "ControlBuffer.h"

dc::ControlBuffer::Iterator dc::ControlBuffer::Iterator::invalid;
dc::ControlBuffer dc::ControlBuffer::Iterator::_invalidBuffer(0);

dc::ControlMessage::ControlMessage() : type(Type::Trigger), sampleOffset(0)
{
	noParams = {};
}

dc::ControlMessage::ControlMessage(Type type, size_t sampleOffset) : type(type), sampleOffset(sampleOffset)
{
	switch (type)
	{
	case Type::Trigger:
		noParams = {};
		break;
	case Type::Note: 
		noteParams = { 0, 1.0f };
		break;
	case Type::Float: 
		floatParams = { 0.0f };
		break;
	default:
		noParams = {};
		break;
	}
}

dc::ControlBuffer::Iterator::Iterator(ControlBuffer& buffer) : _buffer(buffer)
{
}

bool dc::ControlBuffer::Iterator::next(ControlMessage& messageOut)
{
	if (_buffer._messages.empty() || _next >= _buffer._messages.size())
	{
		return false;
	}
	messageOut = _buffer._messages[_next++];
	return true;
}

dc::ControlBuffer::Iterator::Iterator() : _buffer(_invalidBuffer)
{
}

dc::ControlBuffer::ControlBuffer(size_t maxSize) : _maxSize(maxSize)
{
	_messages.reserve(_maxSize);
}

void dc::ControlBuffer::insert(ControlMessage message)
{
	if (_messages.size() < _maxSize)
	{
		// keep the buffer in order of sample offset
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
}

void dc::ControlBuffer::merge(ControlBuffer& other)
{
	Iterator otherIt(other);
	ControlMessage msg;
	while (otherIt.next(msg))
	{
		insert(msg);
	}
}

void dc::ControlBuffer::clear()
{
	_messages.clear();
}
