#pragma once
#include <iostream>
#include <vector>
#include <string>

//https://stackoverflow.com/questions/22277960/get-a-char-from-an-ostream-without-copying

//Now str() is simple. It returns a pointer to the underlying buffer of the auxillary buffer:
//The rest of the functions are the usual implementations for a stream buffer. showmanyc() should return the size of the auxiliary buffer (aux is just a running total of the entire buffer, buffer on the other hand is the size specified at construction).
//For example, here is overflow(), which should update both buffers at same time but still treat buffer as the primary buffer :

//flush() is used to copy the contents of buffer to the stream (os_), and sync() should be overrided to call flush() too.

//xsputn also needs to be overrided to write to aux as well :

/*
It can be used like this:

int main()
{
raw_ostream rostr(std::cout);
rostr << "Hello, World " << 123 << true << false;

auto& buf = rostr.str();
std::cout << buf;
}
*/

/*better example:
std::ofstream out("out.txt");
raw_ostream rostr(out);
rostr << "Hello, World " << 123 << std::boolalpha << true << false;

auto& buf = rostr.str();
std::cout << "Content of the buffer: " << buf;
*/

class raw_buffer : public std::streambuf
{
public:
	raw_buffer(std::ostream& os, int buf_size = 256);
	int_type overflow(int_type c) override;
	std::streamsize showmanyc() override;
	std::streamsize xsputn(const char_type*, std::streamsize) override;
	int sync() override;
	bool flush();
	std::string& str();
	virtual ~raw_buffer();
private:
	std::ostream& os_;
	std::vector<char> buffer;
	std::string aux;
};

class raw_ostream : private virtual raw_buffer
	, public std::ostream
{
public:
	raw_ostream(std::ostream& os) : raw_buffer(os)
		, std::ostream(this)
	{ }

	std::string& str()
	{
		return this->raw_buffer::str();
	}

	std::streamsize count()
	{
		return this->str().size();
	}
};


class outbuf : public std::streambuf
{
protected:
	/* central output function
	* - print characters in uppercase mode
	*/
	/*
	virtual int_type overflow(int_type c) {
	if (c != EOF) {
	// convert lowercase to uppercase
	c = std::toupper(static_cast<char>(c), getloc());

	// and write the character to the standard output
	if (putchar(c) == EOF) {
	return EOF;
	}
	}
	return c;
	}
	*/
};

//Dylan's C style buffer class
class SmallForwardBufferNoFreeOnExitScope
{
public:
	SmallForwardBufferNoFreeOnExitScope::SmallForwardBufferNoFreeOnExitScope(size_t initialsize = 0)
	{
		buffer = 0;
		position = 0;
		if (initialsize)
			buffer = malloc(initialsize);
		buffersize = initialsize;
	}
	void write(void* data, size_t srcsize)
	{
		size_t newposition = position + srcsize;
		if (newposition >= buffersize)
		{
			const size_t newsize = buffersize + srcsize;
			buffer = realloc(buffer, newsize);
			buffersize = newsize;
		}
		memcpy((void*)((uintptr_t)buffer + position), data, srcsize);
		position += srcsize;
	}
	template <class T>
	void write(T data)
	{
		size_t newposition = position + sizeof(data);
		if (newposition >= buffersize)
		{
			const size_t newsize = buffersize + sizeof(data);
			buffer = realloc(buffer, newsize);
			buffersize = newsize;
		}
		*(T*)((uintptr_t)buffer + position) = data;
		position += sizeof(data);
	}
	size_t getsize() const { return position; }
	void* getdata() const { return buffer; }
	void freebuffer() { if (buffersize) free(buffer); }
	void forcefreebuffer() { free(buffer); }
private:
	void* buffer;
	size_t position;
	size_t buffersize;
};

class SmallForwardBuffer
{
public:
	SmallForwardBuffer::SmallForwardBuffer(size_t initialsize = 0)
	{
		buffer = 0;
		position = 0;
		buffersize = 0;
		if (initialsize && (buffer = malloc(initialsize)))
			buffersize = initialsize;
	}
	SmallForwardBuffer::~SmallForwardBuffer()
	{
		if (buffersize)
			free(buffer);
	}
	void write(void* data, size_t srcsize)
	{
		size_t newposition = position + srcsize;
		if (newposition >= buffersize)
		{
			const size_t newsize = buffersize + srcsize;
			buffer = realloc(buffer, newsize);
			buffersize = newsize;
			if (!buffer)
			{
				buffersize = 0;
				position = 0;
			}
		}
		memcpy((void*)((uintptr_t)buffer + position), data, srcsize);
		position += srcsize;
	}
	template <class T>
	void write(T& data)
	{
		size_t newposition = position + sizeof(data);
		if (newposition >= buffersize)
		{
			const size_t newsize = buffersize + sizeof(data);
			buffer = realloc(buffer, newsize);
			buffersize = newsize;
			if (!buffer)
			{
				buffersize = 0;
				position = 0;
			}
		}
		*(T*)((uintptr_t)buffer + position) = data;
		position += sizeof(data);
	}
	template <class T>
	void write(const T& data)
	{
		size_t newposition = position + sizeof(data);
		if (newposition >= buffersize)
		{
			const size_t newsize = buffersize + sizeof(data);
			buffer = realloc(buffer, newsize);
			buffersize = newsize;
			if (!buffer)
			{
				buffersize = 0;
				position = 0;
			}
		}
		*(T*)((uintptr_t)buffer + position) = data;
		position += sizeof(data);
	}
	template<typename T> size_t getcount() { return position / sizeof(T); }
	size_t getsize() const { return position; }
	void* getdata() const { return buffer; }
	template<typename T> T* getdata() { return (T*)buffer; }
	template<typename T> T* at(size_t index) { return position ? (T*)(((uintptr_t)buffer + sizeof(T) * index)): (T*)nullptr; }
	template<typename T> T* front() { return position ? (T*)((uintptr_t)buffer) : nullptr; }
	template<typename T> T* back() { return position ? (T*)((uintptr_t)buffer + (position - sizeof(T))) : nullptr; }
	template<typename T> T* begin() { return position ? (T*)((uintptr_t)buffer) : nullptr; }
	template<typename T> T* end() { return position ? (T*)((uintptr_t)buffer + position) : nullptr; }
	template<typename T> T* rbegin() { return position ? (T*)((uintptr_t)buffer + (position - sizeof(T))) : nullptr; }
	template<typename T> T* rend() { return position ? (T*)((uintptr_t)buffer - sizeof(T)) : nullptr; }
	template<typename T> T* erase(T* item)
	{
		uintptr_t start = (uintptr_t)buffer;
		size_t startbytes = (uintptr_t)item - start;
		uintptr_t itemplus = (uintptr_t)item + sizeof(T);
		if (position > itemplus)
		{
			uintptr_t back = (uintptr_t)end<T>();
			size_t endbytes = (uintptr_t)back - itemplus;
			auto newbuffersize = startbytes + endbytes;
			void* newbuffer = malloc(newbuffersize);
			memmove(newbuffer, (const void*)start, startbytes);
			void* newitemplus = (void*)((uintptr_t)newbuffer + startbytes);
			memmove(newitemplus, (void*)itemplus, endbytes);
			position -= sizeof(T);
			buffersize = newbuffersize;
			free(buffer);
			buffer = newbuffer;
			return (T*)newitemplus;
		}
		else
		{
			pop_back<T>();
			return back<T>();
		}
	}
	template<typename T> void pop_front()
	{
		if (buffersize)
		{
			const auto sz = buffersize - sizeof(T);
			if (sz > 0)
			{
				void* newstart = (void*)((uintptr_t)buffer + sizeof(T));
				const auto sz = buffersize - sizeof(T);
				void* newbuffer = malloc(sz);
				memmove(newbuffer, newstart, sz);
				if (newbuffer)
				{
					memmove(newbuffer, newstart, sz);
					free(buffer);
					buffer = newbuffer;
					buffersize = sz;
					position -= sizeof(T);
				}
				else
				{
					free(buffer);
					buffersize = 0;
					position = 0;
				}
			}
			else
			{
				free(buffer);
				buffer = nullptr;
				position = 0;
				buffersize = 0;
			}
		}
	}
	template<typename T> void pop_back()
	{
		if (buffersize)
		{
			const auto sz = buffersize - sizeof(T);
			if (sz > 0)
			{
				buffer = realloc(buffer, sz);
				if (buffer)
				{
					buffersize = sz;
					position -= sizeof(T);
				}
				else
				{
					buffersize = 0;
					position = 0;
				}
			}
			else
			{
				free(buffer);
				buffer = nullptr;
				position = 0;
				buffersize = 0;
			}
		}
	}
public:
	void* buffer;
	size_t position;
	size_t buffersize;
};