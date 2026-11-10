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

#include "core/serialization/serialization.hpp"

#include "core/serialization/compression.hpp"
#include "core/common/hashing.hpp"
#include "core/data/istream.hpp"
#include "core/data/ostream.hpp"
#include "core/data/string.hpp"
#include "core/io/file_system.hpp"
#include "core/io/log.hpp"
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

			if (!outFile.good())
			{
				SFG_ERR("failed writing to file! {0}", target_file);
				return false;
			}
		}
		else
		{
			SFG_ERR("failed writing to file! {0}", target_file);
			return false;
		}

		return true;
	}

	bool serializer_t::write_files_transactional(span_t<const serialized_file_t> files)
	{
		struct staged_file_t
		{
			string_t temporary	   = {};
			string_t backup		   = {};
			bool	 existed	   = false;
			bool	 retain_backup = false;
		};

		vector_t<staged_file_t> staged	  = {};
		size_t					committed = 0;
		bool					succeeded = true;

		staged.resize(files.size);

		// prepare all files before replacing any of them
		for (size_t i = 0; i < files.size; ++i)
		{
			staged_file_t& file	  = staged[i];
			const string_t suffix = "." + std::to_string(hashing_t::generate_guid64());

			file.temporary = files.data[i].path + suffix + ".tmp";
			file.backup	   = files.data[i].path + suffix + ".bak";
			file.existed   = file_system_t::exists(files.data[i].path);

			const bool copy = file_system_t::copy_file(files.data[i].path, file.backup.c_str());

			if (file.existed)
			{
				if (!copy)
				{
					succeeded = false;
					break;
				}

				if (!write_to_file(files.data[i].contents, file.temporary.c_str()))
					succeeded = false;

				break;
			}
		}

		if (succeeded)
		{
			for (; committed < files.size; ++committed)
			{
				if (!file_system_t::replace_file(staged[committed].temporary.c_str(), files.data[committed].path))
				{
					succeeded = false;
					break;
				}
			}
		}

		// put earlier files back if a later replacement failed
		if (!succeeded)
		{
			while (committed != 0)
			{
				--committed;

				staged_file_t& file = staged[committed];

				if (file.existed)
				{
					if (!file_system_t::replace_file(file.backup.c_str(), files.data[committed].path))
					{
						file.retain_backup = true;
						SFG_ERR("failed to roll back {0}; original file retained at {1}", files.data[committed].path, file.backup);
					}
				}
				else
				{
					std::error_code error = {};

					std::filesystem::remove(files.data[committed].path, error);

					if (error)
						SFG_ERR("failed to remove new file during rollback {0}: {1}", files.data[committed].path, error.message());
				}
			}
		}

		for (const staged_file_t& file : staged)
		{
			const string_t* paths[] = {&file.temporary, &file.backup};

			for (const string_t* path : paths)
			{
				if (path->empty() || (path == &file.backup && file.retain_backup))
					continue;

				std::error_code error = {};

				std::filesystem::remove(*path, error);

				if (error)
					SFG_ERR("failed to remove staged file {0}: {1}", *path, error.message());
			}
		}

		return succeeded;
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
