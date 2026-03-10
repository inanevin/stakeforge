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

#include "serialization/serialization.hpp"
#include "io/log.hpp"
#include "io/file_system.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"
#include "compressor.hpp"

#include <fstream>
#include <filesystem>

namespace SFG
{
	bool serialization::write_to_file(string_view fileInput, const char* target_file)
	{
		std::ofstream outFile(target_file);

		if (outFile.is_open())
		{
			outFile.write(fileInput.data(), fileInput.size());
			outFile.close();
		}
		else
		{
			SFG_ERR("Failed writing to file! {0}", target_file);
			return false;
		}

		return true;
	}

	bool serialization::save_to_file(const char* path, ostream& stream)
	{
		if (file_system::exists(path))
			file_system::delete_file(path);

		std::ofstream wf(path, std::ios::out | std::ios::binary);

		if (!wf)
		{
			SFG_ERR("[Serialization] -> Could not open file for writing! {0}", path);
			return false;
		}

		ostream compressed = compressor::compress(stream);
		compressed.write_to_ofstream(wf);
		wf.close();
		compressed.destroy();

		if (!wf.good())
		{
			SFG_ERR("[Serialization] -> Error occured while writing the file! {0}", path);
			return false;
		}

		return true;
	}

	istream serialization::load_from_file(const char* path)
	{
		if (!file_system::exists(path))
		{
			SFG_ERR("[Serialization] -> File doesn't exists: {0}", path);
			return {};
		}

		std::ifstream rf(path, std::ios::out | std::ios::binary);

		if (!rf)
		{
			SFG_ERR("[Serialization] -> Could not open file for reading! {0}", path);
			return istream();
		}

		auto size = std::filesystem::file_size(path);

		// Create
		istream readStream;
		readStream.create(nullptr, size);
		readStream.read_from_ifstream(rf);
		rf.close();

		if (!rf.good())
		{
			SFG_ERR("[Serialization] -> Error occured while reading the file! {0}", path);
			readStream.destroy();
			return {};
		}

		istream decompressedStream = compressor::decompress(readStream);
		readStream.destroy();
		return decompressedStream;
	}

}
