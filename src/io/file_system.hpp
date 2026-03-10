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

#include "common/size_definitions.hpp"
#include "data/string.hpp"

namespace std
{
	namespace filesystem
	{
		class path;
	}
}
namespace SFG
{
	class file_system
	{
	public:
		static bool	  delete_file(const char* path);
		static bool	  create_directory(const char* path);
		static bool	  delete_directory(const char* path);
		static bool	  is_directory(const char* path);
		static bool	  change_directory_name(const char* oldPath, const char* new_path);
		static bool	  exists(const char* path);
		static string get_last_modified_date(const char* path);
		static u64	  get_last_modified_ticks(const char* path) noexcept;
		static u64	  get_last_modified_ticks(const std::filesystem::path& path) noexcept;
		static string get_directory_of_file(const char* path);
		static string remove_extensions_from_path(const string& filename);
		static string get_filename_and_extension_from_path(const string& filename);
		static string get_file_extension(const string& file);
		static string get_filename_from_path(const string& file);
		static string get_last_folder_from_path(const char* path);
		static string read_file_as_string(const char* file);
		static string get_running_directory();
		static string get_user_directory();
		static void	  fix_path(string& str);
		static string duplicate(const char* path);
		static string get_relative(const char* src, const char* target);
		static string get_system_time_str();
		static string get_time_str_from_microseconds(i64 microseconds);
		static void	  read_file(const char* file_path, char*& out_data, size_t& out_size);
		static void	  perform_move(const char* target_file, const char* target_dir);
		static void	  get_sys_time_ints(i32& hours, i32& minutes, i32& seconds);
		static void	  copy_directory(const char* copyDir, const char* target_parent_folder);
		static void	  copy_file_to_directory(const char* file, const char* target_parent_folder);
	};

} // namespace SFG
