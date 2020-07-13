//#ifndef NOCRYPTLIB

#include "precompiled.h"
#include "compression.h"

//#ifndef MUTINY_FRAMEWORK
//#include "cryptlib.h"
//#include "gzip.h"
//#else
#if _MSC_VER <= 1900
#include "G:\developerframework_old\Libs\Headers\cryptlib.h"
#include "G:\developerframework_old\Libs\Headers\gzip.h"
#else
#include "C:\Developer\Sync\Framework\CryptoPP\cryptlib.h"
#include "C:\Developer\Sync\Framework\CryptoPP\gzip.h"
#endif
//#endif

void tcp_compressor::compress(const char *data, std::uint32_t size)
{
	//compressed_.clear();
	//compressed_ = MutinyFrame::cCryptInstance(MutinyFrame::CRYPT_TYPE_COMPRESS, data).Execute();

	compressed_.clear();
	//if (sink)
	//	delete sink;
	auto sink = (void*)(new CryptoPP::StringSink(compressed_));
	CryptoPP::Gzip zipper((CryptoPP::StringSink*)sink);
	zipper.Put(reinterpret_cast<const /*CryptoPP::*/byte*>(data), size);
	zipper.MessageEnd();
}

void tcp_decompressor::decompress(const char *data, std::uint32_t size)
{
	//decompressed_.clear();
	//decompressed_ = MutinyFrame::cCryptInstance(MutinyFrame::CRYPT_TYPE_DECOMPRESS, data).Execute();

	decompressed_.clear();
	//if (sink)
	//	delete sink;
	auto sink = (void*)(new CryptoPP::StringSink(decompressed_));
	CryptoPP::Gunzip unzipper((CryptoPP::StringSink*)sink);
	unzipper.Put(reinterpret_cast<const /*CryptoPP::*/byte*>(data), size);
	unzipper.MessageEnd();

}

//#endif