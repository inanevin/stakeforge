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
#include <sfg/common/hashing.hpp>
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/data/string.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <fstream>
#include <filesystem>
#include <limits>

namespace sfg
{
	bool serializer_t::write_to_file(string_view_t file_input, const char* target_file)
	{
		std::ofstream outFile(target_file, std::ios::out | std::ios::binary);

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

		wf.write(reinterpret_cast<const char*>(stream.get_raw()), stream.get_size());
		wf.close();

		if (!wf.good())
		{
			SFG_ERR("error occured while writing the file! {0}", path);
			return false;
		}

		return true;
	}

	bool serializer_t::save_to_file_atomic(const char* path, const ostream_t& stream)
	{
		string_t temporary_path = path;
		temporary_path += ".";
		temporary_path += std::to_string(hashing_t::generate_guid64());
		temporary_path += ".tmp";

		if (!save_to_file(temporary_path.c_str(), stream))
		{
			if (file_system_t::exists(temporary_path.c_str()))
				file_system_t::delete_file(temporary_path.c_str());

			return false;
		}

		if (!file_system_t::replace_file(temporary_path.c_str(), path))
		{
			file_system_t::delete_file(temporary_path.c_str());
			return false;
		}

		return true;
	}

	bool serializer_t::save_to_file_compressed(const char* path, const ostream_t& stream)
	{
		ostream_t compressed = compressor_t::compress(stream);
		if (compressed.get_size() == 0)
			return false;

		return save_to_file(path, compressed);
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

		return read_stream;
	}

	istream_t serializer_t::load_from_file_slice(const char* path, u64 offset, u64 size)
	{
		if (!file_system_t::exists(path))
		{
			SFG_ERR("file doesn't exists: {0}", path);
			return {};
		}

		const u64 file_size = static_cast<u64>(std::filesystem::file_size(path));
		if (offset > file_size || size > file_size - offset)
		{
			SFG_ERR("file slice out of range: {0}", path);
			return {};
		}

		if (offset > static_cast<u64>((std::numeric_limits<std::streamoff>::max)()))
		{
			SFG_ERR("file slice offset is too large: {0}", path);
			return {};
		}

		if (size > static_cast<u64>((std::numeric_limits<size_t>::max)()))
		{
			SFG_ERR("file slice size is too large: {0}", path);
			return {};
		}

		if (size == 0)
			return {};

		std::ifstream rf(path, std::ios::in | std::ios::binary);

		if (!rf)
		{
			SFG_ERR("could not open file for reading! {0}", path);
			return istream_t();
		}

		rf.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
		if (!rf.good())
		{
			SFG_ERR("error occured while seeking the file! {0}", path);
			return {};
		}

		istream_t read_stream;
		read_stream.create(nullptr, static_cast<size_t>(size));
		read_stream.read_from_ifstream(rf);
		rf.close();

		if (!rf.good())
		{
			SFG_ERR("error occured while reading the file! {0}", path);
			read_stream.destroy();
			return {};
		}

		return read_stream;
	}

	istream_t serializer_t::load_from_file_compressed(const char* path)
	{
		istream_t read_stream = load_from_file(path);
		if (read_stream.empty())
			return {};

		return compressor_t::decompress(read_stream);
	}

}
