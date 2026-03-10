/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "compressor.hpp"
#include "io/log.hpp"
#include "data/istream.hpp"
#include "data/ostream.hpp"
#include <lz4/lz4.h>

namespace SFG
{
	size_t estimate_decompressed_size(size_t compressedSize)
	{
		// return (compressedSize << 8) - compressedSize - 2526;
		return 255 * compressedSize + 24;
	}

	ostream compressor::compress(ostream& stream)
	{
		const u32 streamSize	   = static_cast<u32>(stream.get_size());
		const u8  shouldCompress   = (streamSize < 150000000 && streamSize > 750000) ? 1 : 0;
		const u32 uncompressedSize = streamSize + sizeof(u8) + sizeof(u32);

		stream << shouldCompress;
		stream << uncompressedSize;

		if (!shouldCompress)
		{
			ostream compressed;
			compressed.create(stream.get_size());
			compressed.write_raw(stream.get_raw(), stream.get_size());
			return compressed;
		}

		const int size			= static_cast<int>(stream.get_size());
		const int compressBound = LZ4_compressBound(size);

		// Create stream capable of holding max compressed bytes.
		ostream compressedStream = ostream();
		compressedStream.create(compressBound);
		char* dest		   = (char*)compressedStream.get_raw();
		char* data		   = (char*)stream.get_raw();
		int	  bytesWritten = LZ4_compress_default(data, dest, size, compressBound);

		if (bytesWritten == 0)
		{
			SFG_ERR("[compressor] -> LZ4 compression failed!");
		}

		compressedStream.shrink(static_cast<size_t>(bytesWritten));
		return compressedStream;
	}

	istream compressor::decompress(istream& stream)
	{
		// Read uncompressed size of archive.
		u8	shouldDecompress = 0;
		u32 uncompressedSize = 0;
		stream.seek(stream.get_size() - sizeof(u32) - sizeof(u8));
		stream.read(shouldDecompress);
		stream.read(uncompressedSize);
		stream.seek(0);

		if (!shouldDecompress)
		{
			istream copy;
			copy.create(stream.get_raw(), stream.get_size() - sizeof(u32) - sizeof(u8));
			return copy;
		}

		const size_t size				= stream.get_size();
		istream		 decompressedStream = istream();
		decompressedStream.create(nullptr, uncompressedSize);
		void*	  src			   = stream.get_raw();
		void*	  ptr			   = decompressedStream.get_raw();
		const int decompressedSize = LZ4_decompress_safe((char*)src, (char*)ptr, static_cast<int>(size), static_cast<int>(uncompressedSize));
		decompressedStream.shrink(static_cast<size_t>(decompressedSize) - sizeof(u32) - sizeof(u8));
		return decompressedStream;
	}
}
