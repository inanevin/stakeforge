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
#include "data/vector.hpp"

namespace std
{
	namespace filesystem
	{
		class path;
	}
}

namespace sfg
{
	namespace file_system
	{
		bool	 delete_file(const char* path);
		bool	 create_directory(const char* path);
		bool	 delete_directory(const char* path);
		bool	 is_directory(const char* path);
		bool	 change_directory_name(const char* oldPath, const char* new_path);
		bool	 exists(const char* path);
		bool	 is_absolute_path(const char* path);
		string_t get_last_modified_date(const char* path);
		u64		 get_last_modified_ticks(const char* path) noexcept;
		u64		 get_last_modified_ticks(const std::filesystem::path& path) noexcept;
		string_t get_absolute_path(const char* path);
		string_t get_directory_of_file(const char* path);
		string_t remove_extensions_from_path(const string_t& filename);
		string_t get_filename_and_extension_from_path(const string_t& filename);
		string_t get_file_extension(const string_t& file);
		string_t get_filename_from_path(const string_t& file);
		string_t get_last_folder_from_path(const char* path);
		string_t read_file_as_string(const char* file);
		string_t get_running_directory();
		string_t get_user_directory();
		void	 fix_path(string_t& str);
		string_t duplicate(const char* path);
		string_t get_relative(const char* src, const char* target);
		string_t get_system_time_str();
		string_t get_time_str_from_microseconds(i64 microseconds);
		void	 read_file(const char* file_path, char*& out_data, size_t& out_size);
		void	 perform_move(const char* target_file, const char* target_dir);
		void	 get_sys_time_ints(i32& hours, i32& minutes, i32& seconds);
		void	 copy_directory(const char* copyDir, const char* target_parent_folder);
		void	 copy_file_to_directory(const char* file, const char* target_parent_folder);
		void	 get_files_recursive(const char* directory, vector_t<string_t>& out_files);
	};

}
