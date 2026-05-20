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

#include "serialization.hpp"
#include "compression.hpp"
#include <sfg/io/log.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/data/istream.hpp>
#include <fstream>
#include <filesystem>

namespace sfg
{
	bool serializer_t::write_to_file(string_view_t file_input, const char* target_file)
	{
		std::ofstream outFile(target_file);

		if (outFile.is_open())
		{
			outFile.write(file_input.data(), file_input.size());
			outFile.close();
		}
		else
		{
			SFG_ERR("failed writing to file! {0}", target_file);
			return false;
		}

		return true;
	}

	bool serializer_t::save_to_file(const char* path, const ostream_t& stream)
	{
		if (file_system_t::exists(path))
			file_system_t::delete_file(path);

		std::ofstream wf(path, std::ios::out | std::ios::binary);

		if (!wf)
		{
			SFG_ERR("could not open file for writing! {0}", path);
			return false;
		}

		ostream_t compressed = compressor_t::compress(stream);
		compressed.write_to_ofstream(wf);
		wf.close();

		if (!wf.good())
		{
			SFG_ERR("error occured while writing the file! {0}", path);
			return false;
		}

		return true;
	}

	istream_t serializer_t::load_from_file(const char* path)
	{
		if (!file_system_t::exists(path))
		{
			SFG_ERR("file doesn't exists: {0}", path);
			return {};
		}

		std::ifstream rf(path, std::ios::in | std::ios::binary);

		if (!rf)
		{
			SFG_ERR("could not open file for reading! {0}", path);
			return istream_t();
		}

		auto size = std::filesystem::file_size(path);

		// Create
		istream_t read_stream;
		read_stream.create(nullptr, size);
		read_stream.read_from_ifstream(rf);
		rf.close();

		if (!rf.good())
		{
			SFG_ERR("error occured while reading the file! {0}", path);
			read_stream.destroy();
			return {};
		}

		istream_t decompressedStream = compressor_t::decompress(read_stream);
		return decompressedStream;
	}

}
