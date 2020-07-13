//#ifndef NOCRYPTLIB

#pragma once
#include <string>

class tcp_compressor
{
public:
	tcp_compressor()
	{
		//sink = nullptr;
	}
	~tcp_compressor()
	{
		//if (sink)
		//	delete sink;
	}
	void compress(const char *data, std::uint32_t size);
	std::string &get_data()
	{
		return compressed_;
	}
private:
	std::string compressed_;
	//void* sink;
};

class tcp_decompressor
{
public:
	tcp_decompressor()
	{
		//sink = nullptr;
	}
	~tcp_decompressor()
	{
		//	if (sink)
		//		delete sink;
	}
	void decompress(const char *data, std::uint32_t size);
	std::string &get_data()
	{
		return decompressed_;
	}
private:
	//void* sink;
	std::string decompressed_;
};

//#endif