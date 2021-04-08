#pragma once
#include <cstring>

class RingBuffer
{
	private:
		char* _data;
		uint64_t _size;
		uint64_t _widx;
		uint64_t _ridx;
	public:
		RingBuffer(uint64_t size):
			_data(new char[size]),_size(size),_widx(0),_ridx(0){}
		~RingBuffer(){
			delete [] _data;
		}

		uint64_t write(const char* data, uint64_t size)
		{
			if(writableSize() < size)
			{
				size = writableSize();
			}
			memcpy(&_data[_widx], data, size);
			_widx =(_widx + size) % _size;
			//printf("_ridx, _widx=%u, %u\n", _ridx,_widx);
			return size;
		}

		uint64_t read(char* data, uint64_t size)
		{
			if(readableSize() < size)
			{
				size = readableSize();
			}

			memcpy(data, &_data[_ridx], size);
			_ridx = (_ridx + size) % _size;
			if(size > 0){
			//printf("_ridx, _widx=%u, %u\n", _ridx,_widx);

			}
			return size;
		}

		uint64_t writableSize()
		{
			if(_widx >= _ridx)
			{
				return (_size - _widx) + _ridx;
			}
			else
			{
				return _ridx - _widx;
			}
		}

		uint64_t readableSize()
		{
			if(_ridx <= _widx)
			{
				return _widx - _ridx;
			}
			else
			{
				return (_size - _ridx) + _widx;
			}
		}
};
