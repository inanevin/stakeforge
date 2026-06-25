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

#pragma once

#include <sfg/common/size_definitions.hpp>
#include <sfg/data/hash_map.hpp>
#include <sfg/data/string.hpp>

namespace sfg
{
	class ostream_t;

	struct resource_map_info_t
	{
		size_t offset = 0;
		size_t size	  = 0;
	};

	class resource_file_system_t final
	{
	public:
		resource_file_system_t()										 = default;
		~resource_file_system_t()										 = default;
		resource_file_system_t(const resource_file_system_t&)			 = delete;
		resource_file_system_t& operator=(const resource_file_system_t&) = delete;

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void set_mode_directory(const char* directory_path, const char* engine_cache);
		void set_mode_filepack(const char* path_to_file_pack, const hash_map_t<u64, resource_map_info_t>& resource_map);
		bool read_resource(u64 hash, size_t offset, size_t size, ostream_t& out);

	private:
		enum class mode_e : u8
		{
			none,
			directory,
			filepack,
		};

		bool read_file_range(const char* path, size_t offset, size_t size, ostream_t& out);

	private:
		hash_map_t<u64, resource_map_info_t> _resource_map;
		string_t							 _directory_path;
		string_t							 _engine_cache;
		string_t							 _file_pack_path;
		mode_e								 _mode = mode_e::none;
	};
}
