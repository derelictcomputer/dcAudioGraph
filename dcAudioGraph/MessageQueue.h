#pragma once

#include <atomic>

namespace dc
{
template <class MessageType>
class  MessageQueue final
{
public:
	explicit MessageQueue(size_t maxSize) : _maxSize(maxSize)
	{
		_data = static_cast<MessageType*>(malloc(maxSize * sizeof(MessageType)));
	}

	~MessageQueue()
	{
		free(_data);
	}

	// no copy/move
	MessageQueue(const MessageQueue&) = delete;
	MessageQueue& operator=(const MessageQueue&) = delete;
	MessageQueue(MessageQueue&&) = delete;
	MessageQueue& operator=(MessageQueue&&) = delete;

	bool empty() const { return _size.load() == 0; }
	bool full() const { return _size.load() >= _maxSize; }
	size_t numMessages() const { return _size.load(); }

	bool push(const MessageType& msg)
	{
		if (_size.load(std::memory_order_acquire) < _maxSize)
		{
			_data[_tail] = msg;
			_tail = (_tail + 1) % _maxSize;
			_size.fetch_add(1, std::memory_order_release);
			return true;
		}
		return false;
	}

	bool pop(MessageType& msg)
	{
		if (_size.load(std::memory_order_acquire) > 0)
		{
			msg = _data[_head];
			_head = (_head + 1) % _maxSize;
			_size.fetch_sub(1, std::memory_order_release);
			return true;
		}
		return false;
	}

private:
	MessageType* _data = nullptr;
	const size_t _maxSize;
	std::atomic<size_t> _size{ 0 };
	size_t _head = 0;
	size_t _tail = 0;
};
}
