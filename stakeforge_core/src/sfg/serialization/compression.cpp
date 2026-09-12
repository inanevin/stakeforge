/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.
*/

#include "compression.hpp"
#include <sfg/io/log.hpp>
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <lz4/lz4.h>

namespace sfg
{
	namespace
	{
		size_t estimate_decompressed_size(size_t compressedSize)
		{
			// return (compressedSize << 8) - compressedSize - 2526;
			return 255 * compressedSize + 24;
		}
	}

	ostream_t compressor_t::compress(const ostream_t& stream)
	{
		const u32 streamSize	   = static_cast<u32>(stream.get_size());
		const u8  shouldCompress   = (streamSize < 150000000 && streamSize > 750000) ? 1 : 0;
		const u32 uncompressedSize = streamSize;

		if (!shouldCompress)
		{
			ostream_t compressed;
			compressed.create(sizeof(u8) + sizeof(u32) + stream.get_size());
			compressed << shouldCompress;
			compressed << uncompressedSize;
			compressed.write_raw(stream.get_raw(), stream.get_size());
			return compressed;
		}

		const int size			= static_cast<int>(streamSize);
		const int compressBound = LZ4_compressBound(size);

		ostream_t compressedStream = ostream_t();
		compressedStream.create(sizeof(u8) + sizeof(u32) + static_cast<size_t>(compressBound));
		compressedStream << shouldCompress;
		compressedStream << uncompressedSize;

		char* dest		   = reinterpret_cast<char*>(compressedStream.get_raw() + compressedStream.get_size());
		char* data		   = reinterpret_cast<char*>(stream.get_raw());
		int	  bytesWritten = LZ4_compress_default(data, dest, size, compressBound);

		if (bytesWritten == 0)
		{
			SFG_ERR("LZ4 compression failed!");
			return {};
		}

		compressedStream.shrink(sizeof(u8) + sizeof(u32) + static_cast<size_t>(bytesWritten));
		return compressedStream;
	}

	istream_t compressor_t::decompress(istream_t& stream)
	{
		u8	shouldDecompress = 0;
		u32 uncompressedSize = 0;
		stream >> shouldDecompress;
		stream >> uncompressedSize;

		if (!shouldDecompress)
		{
			istream_t copy;
			copy.create(stream.get_data_current(), stream.get_size() - stream.tellg());
			return copy;
		}

		const size_t size				 = stream.get_size() - stream.tellg();
		istream_t	 decompressed_stream = istream_t();
		decompressed_stream.create(nullptr, uncompressedSize);
		void*	  src			   = stream.get_data_current();
		void*	  ptr			   = decompressed_stream.get_raw();
		const int decompressedSize = LZ4_decompress_safe((char*)src, (char*)ptr, static_cast<int>(size), static_cast<int>(uncompressedSize));
		if (decompressedSize < 0 || static_cast<u32>(decompressedSize) != uncompressedSize)
		{
			SFG_ERR("LZ4 decompression failed!");
			return {};
		}

		return decompressed_stream;
	}
}
