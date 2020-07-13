#include "precompiled.h"
#include "raw_buffer.h"

//https://stackoverflow.com/questions/22277960/get-a-char-from-an-ostream-without-copying

raw_buffer::raw_buffer(std::ostream& os, int buf_size)
	: os_(os)
	, buffer(buf_size)
{
	this->setp(buffer.data(), buffer.data() + buffer.size() - 1);
}

std::streamsize raw_buffer::showmanyc()
{
	return epptr() - pptr();
}

bool raw_buffer::flush()
{
	std::ptrdiff_t n = pptr() - pbase();
	return bool(os_.write(buffer.data(), n));
}

int raw_buffer::sync()
{
	return flush() ? 0 : -1;
}

std::string& raw_buffer::str()
{
	return aux;
}

raw_buffer::int_type raw_buffer::overflow(raw_buffer::int_type c)
{
	if (os_ && !traits_type::eq_int_type(c, traits_type::eof()))
	{
		aux += *this->pptr() = traits_type::to_char_type(c);
		this->pbump(1);

		if (flush())
		{
			this->pbump(-(this->pptr() - this->pbase()));
			this->setp(this->buffer.data(),
				this->buffer.data() + this->buffer.size());
			return c;
		}
	}
	return traits_type::eof();
}

std::streamsize raw_buffer::xsputn(const raw_buffer::char_type* str, std::streamsize count)
{
	for (int i = 0; i < count; ++i)
	{
		if (traits_type::eq_int_type(this->sputc(str[i]), traits_type::eof()))
			return i;
		else
			aux += str[i];
	}
	return count;
}

raw_buffer::~raw_buffer()
{
	this->flush();
}