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

#pragma once

#include <sfg/common/size_definitions.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>

namespace std
{
	namespace filesystem
	{
		class path;
	}
}

namespace sfg
{
	enum class file_system_entry_type_e : u8
	{
		file,
		directory,
	};

	struct file_system_entry_t
	{
		string_t				 path;
		file_system_entry_type_e type = file_system_entry_type_e::file;
	};

	class file_system_t
	{
	public:
		static bool		delete_file(const char* path);
		static bool		create_directory(const char* path);
		static bool		ensure_directory(const char* path);
		static bool		delete_directory(const char* path);
		static bool		is_directory(const char* path);
		static bool		change_directory_name(const char* oldPath, const char* new_path);
		static bool		exists(const char* path);
		static u64		get_file_size(const char* path);
		static bool		is_absolute_path(const char* path);
		static string_t get_last_modified_date(const char* path);
		static u64		get_last_modified_ticks(const char* path) noexcept;
		static u64		get_last_modified_ticks(const std::filesystem::path& path) noexcept;
		static string_t get_absolute_path(const char* path);
		static string_t get_directory_of_file(const char* path);
		static string_t remove_extensions_from_path(const string_t& filename);
		static string_t get_filename_and_extension_from_path(const string_t& filename);
		static string_t get_file_extension(const string_t& file);
		static string_t get_filename_from_path(const string_t& file);
		static string_t get_last_folder_from_path(const char* path);
		static string_t read_file_as_string(const char* file);
		static string_t get_running_directory();
		static string_t get_user_directory();
		static void		fix_path(string_t& str);
		static void		fix_path_end_slash(string_t& str);
		static string_t duplicate(const char* path);
		static string_t get_relative(const char* src, const char* target);
		static string_t get_system_time_str();
		static string_t get_system_time_tag_str(u32 count = 1);
		static string_t get_time_str_from_microseconds(i64 microseconds);
		static void		read_file(const char* file_path, char*& out_data, size_t& out_size);
		static void		find_lines_with_keyword(const char* file, const char* keyword, vector_t<string_t>& out_lines);
		static void		perform_move(const char* target_file, const char* target_dir);
		static void		get_sys_time_ints(i32& hours, i32& minutes, i32& seconds);
		static void		copy_directory(const char* copyDir, const char* target_parent_folder);
		static bool		copy_file(const char* file, const char* target_file);
		static bool		replace_file(const char* source_path, const char* target_path);
		static void		copy_file_to_directory(const char* file, const char* target_parent_folder);
		static void		get_files_recursive(const char* directory, vector_t<string_t>& out_files);
		static void		get_entries_recursive(const char* directory, vector_t<file_system_entry_t>& out_entries);
	};
}
