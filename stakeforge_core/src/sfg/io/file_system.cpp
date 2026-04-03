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

#include "file_system.hpp"
#include "log.hpp"
#include "data/string_util.hpp"

#include <filesystem>
#include <fstream>

#ifdef SFG_PLATFORM_OSX
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#endif

#ifdef SFG_PLATFORM_WINDOWS
#include <shlobj.h>
#endif

namespace sfg
{
	namespace file_system
	{

		bool delete_file(const char* path)
		{
			return remove(path);
		}

		bool create_directory(const char* path)
		{
			vector_t<string_t> directories = {};
			string_util::split(directories, path, "/");

			string_t current_path = "";
			for (const auto& dir : directories)
			{
				current_path += dir + "/";

				if (!exists(current_path.c_str()))
				{
					bool success = std::filesystem::create_directory(current_path.c_str());
					if (!success)
					{
						SFG_ERR("Could not create directory: {0}", current_path);
						return false;
					}
				}
			}
			return true;
		}

		bool delete_directory(const char* path)
		{
			try
			{
				bool success = std::filesystem::remove_all(path);
			}
			catch (const std::exception& err)
			{
				SFG_ERR("Could not delete directory: {0}, {1}", path, err.what());
				return false;
			}

			return true;
		}

		bool is_directory(const char* path)
		{
			return std::filesystem::is_directory(path);
		}

		bool change_directory_name(const char* old_path, const char* new_path)
		{
			if (std::rename(old_path, new_path) != 0)
			{
				SFG_ERR("Failed to rename directory! Old Name: {0}, New Name: {1}", old_path, new_path);
				return false;
			}

			return true;
		}

		bool exists(const char* path)
		{
			return std::filesystem::exists(path);
		}

		string_t get_last_modified_date(const char* path)
		{
			std::filesystem::file_time_type ftime  = std::filesystem::last_write_time(path);
			auto							sctp   = std::chrono::time_point_cast<std::chrono::system_clock::duration>(ftime - decltype(ftime)::clock::now() + std::chrono::system_clock::now());
			std::time_t						cftime = std::chrono::system_clock::to_time_t(sctp);
			const string_t					str	   = std::asctime(std::localtime(&cftime));
			return str;
		}

		u64 file_system::get_last_modified_ticks(const char* path) noexcept
		{
			std::error_code					ec;
			std::filesystem::file_time_type ft = std::filesystem::last_write_time(path, ec);
			if (ec)
				return 0;

			using dur = std::chrono::nanoseconds;
			return static_cast<uint64_t>(std::chrono::duration_cast<dur>(ft.time_since_epoch()).count());
		}

		u64 file_system::get_last_modified_ticks(const std::filesystem::path& path) noexcept
		{
			std::error_code					ec;
			std::filesystem::file_time_type ft = std::filesystem::last_write_time(path, ec);
			if (ec)
				return 0;

			using dur = std::chrono::nanoseconds;
			return static_cast<uint64_t>(std::chrono::duration_cast<dur>(ft.time_since_epoch()).count());
		}

		string_t get_directory_of_file(const char* path)
		{
			string_t	 str	 = path;
			const char*	 cstr	 = path;
			unsigned int str_len = (unsigned int)str.length();
			unsigned int end	 = str_len - 1;

			while (end != 0)
			{
				if (cstr[end] == '/')
					break;

				end--;
			}

			if (end == 0)
				return str;

			else
			{
				unsigned int start = 0;
				end				   = end + 1;
				return str.substr(start, end - start).data();
			}
		}

		string_t remove_extensions_from_path(const string_t& fileName)
		{
			const size_t last_index = fileName.find_last_of(".");
			string_t	 path		= fileName.substr(0, last_index);
			fix_path(path);
			return path;
		}

		string_t get_filename_and_extension_from_path(const string_t& fileName)
		{
			string_t path = fileName.substr(fileName.find_last_of("/\\") + 1);
			fix_path(path);
			return path;
		}

		string_t get_file_extension(const string_t& file)
		{
			string_t path = file.substr(file.find_last_of(".") + 1);
			fix_path(path);
			return path;
		}

		string_t get_filename_from_path(const string_t& file)
		{
			return remove_extensions_from_path(get_filename_and_extension_from_path(file));
		}

		string_t get_last_folder_from_path(const char* path)
		{
			string_t fixed_path = path;
			fix_path(fixed_path);
			const size_t last_slash = fixed_path.find_last_of("/\\");

			if (last_slash == fixed_path.size() || last_slash == fixed_path.size() - 1)
				fixed_path = fixed_path.substr(0, last_slash);

			const size_t actualLast = fixed_path.find_last_of("/\\");
			if (actualLast != string_t::npos)
				fixed_path = fixed_path.substr(actualLast + 1, fixed_path.size());
			return fixed_path;
		}

		string_t read_file_as_string(const char* file)
		{
			std::ifstream ifs(file);
			auto		  a = std::istreambuf_iterator<char>(ifs);
			auto		  b = (std::istreambuf_iterator<char>());
			return std::string(a, b);
		}

		void read_file(const char* file_path, char*& out_data, size_t& out_size)
		{
			std::ifstream file(file_path, std::ios::binary);
			if (!file)
			{
				SFG_ERR("Could not open file! {0}", file_path);
				return;
			}

			// Size
			file.seekg(0, std::ios::end);
			std::streampos length = file.tellg();
			file.seekg(0, std::ios::beg);
			out_size = length;

			if (out_size > 0)
			{
				out_data = new char[length];
				file.read(out_data, length);
			}
		}

		string_t get_running_directory()
		{
			try
			{
				auto path = std::filesystem::current_path();
				return path.string();
			}
			catch (std::exception e)
			{
				int a = 5;
				return "";
			}
		}

		string_t get_user_directory()
		{
#ifdef SFG_PLATFORM_WINDOWS
			char path[MAX_PATH];
			if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, path)))
			{
				string_t pp = string_t(path);
				return pp;
			}
			return std::string();
#elif defined SFG_PLATFORM_OSX
			struct passwd* pw	= getpwuid(getuid());
			string_t	   path = string_t(pw->pw_dir);
			return path + "/Library/Application Support/";
#endif
			return "";
		}

		void fix_path(string_t& str)
		{
			string_util::replace_all(str, "\\\\", "/");
			string_util::replace_all(str, "\\", "/");
		}

		namespace
		{
			void SFGCopyFile(const std::filesystem::path& source, const std::filesystem::path& destination)
			{
				try
				{
					std::filesystem::copy_file(source, destination, std::filesystem::copy_options::overwrite_existing);
				}
				catch (std::filesystem::filesystem_error& e)
				{
					SFG_ERR("Error duplicating file! {0}", e.what());
				}
			}

			void SFGCopyDirectory(const std::filesystem::path& source, const std::filesystem::path& destination)
			{
				try
				{
					std::filesystem::create_directory(destination);
					for (const auto& entry_t : std::filesystem::recursive_directory_iterator(source))
					{
						const auto& path			= entry_t.path();
						auto		relativePathStr = path.lexically_relative(source).string();
						std::filesystem::copy(path, destination / relativePathStr, std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive);
					}
				}
				catch (std::filesystem::filesystem_error& e)
				{
					SFG_ERR("Error duplicating directory! {0}", e.what());
				}
			}
		} // namespace

		string_t duplicate(const char* path)
		{
			try
			{
				if (std::filesystem::exists(path))
				{
					const bool	   is_dir		  = file_system::is_directory(path);
					const string_t corrected_path = is_dir ? path : file_system::remove_extensions_from_path(path);
					string_t	   final_path	  = corrected_path + " (Copy)";

					size_t inster_before_ext = final_path.length();
					if (!is_dir)
					{
						final_path += "." + file_system::get_file_extension(path);
					}

					while (file_system::exists(final_path.c_str()))
					{
						final_path.insert(inster_before_ext, " (Copy)");
						inster_before_ext += 7;
					}

					std::filesystem::path destination = final_path;

					if (std::filesystem::is_regular_file(path))
					{
						SFGCopyFile(path, destination);
						return final_path;
					}
					else if (std::filesystem::is_directory(path))
					{
						SFGCopyDirectory(path, destination);
						return final_path;
					}
					else
					{
						SFG_ERR("Unsupported file type! {0}", path);
					}
				}
				else
				{
					SFG_ERR("Path doesn't exist! {0}", path);
				}
			}
			catch (std::filesystem::filesystem_error&)
			{
				SFG_ERR("Exception processing path! {0}", path);
			}

			return "";
		}

		string_t get_relative(const char* src, const char* target)
		{
			std::filesystem::path src_path(src);
			std::filesystem::path dst_path(target);
			std::filesystem::path relative_path = std::filesystem::relative(dst_path, src_path);
			return relative_path.string();
		}

		void perform_move(const char* target_file, const char* target_dir)
		{
			try
			{
				// Create the directory if it doesn't exist
				if (!std::filesystem::exists(target_dir))
				{
					SFG_ERR("Target directory does not exist! {0}", target_dir);
					return;
				}

				// Construct the new path for the file
				std::filesystem::path file		  = std::filesystem::path(target_file);
				std::filesystem::path destination = std::filesystem::path(target_dir) / file.filename();

				// Move (or rename) the file to the new directory
				std::filesystem::rename(target_file, destination);
			}
			catch (const std::filesystem::filesystem_error& e)
			{
				SFG_ERR("::perform_move error {0}", e.what());
			}
		}

		string_t get_system_time_str()
		{
			std::time_t		   now		 = std::time(nullptr);
			std::tm*		   localTime = std::localtime(&now);
			std::ostringstream oss;
			oss << std::setw(2) << std::setfill('0') << localTime->tm_hour << ":" << std::setw(2) << std::setfill('0') << localTime->tm_min << ":" << std::setw(2) << std::setfill('0') << localTime->tm_sec;
			return oss.str();
		}

		void get_sys_time_ints(i32& hours, i32& minutes, i32& seconds)
		{
			std::time_t now		   = std::time(nullptr);
			std::tm*	local_time = std::localtime(&now);
			hours				   = local_time->tm_hour;
			minutes				   = local_time->tm_min;
			seconds				   = local_time->tm_sec;
		}

		string_t get_time_str_from_microseconds(i64 microseconds)
		{
			i64 total_seconds = microseconds / 1000000;

			int hours	= (total_seconds / 3600) % 24; // Wrap-around using modulo 24
			int minutes = (total_seconds / 60) % 60;
			int seconds = total_seconds % 60;

			std::ostringstream oss;
			oss << std::setw(2) << std::setfill('0') << hours << ":" << std::setw(2) << std::setfill('0') << minutes << ":" << std::setw(2) << std::setfill('0') << seconds;

			return oss.str();
		}

		void copy_directory(const char* directory, const char* targetParentFolder)
		{
			namespace fs = std::filesystem;

			try
			{
				// Ensure the source directory exists

				std::filesystem::path source(directory);
				if (!std::filesystem::exists(source))
				{
					throw std::runtime_error("Source directory does not exist or is not a directory.");
				}

				// Create the destination path
				std::filesystem::path destination = std::filesystem::path(targetParentFolder) / source.filename();

				// Check if the destination already exists
				if (std::filesystem::exists(destination))
					std::filesystem::remove_all(destination);

				// Recursively copy the directory and its contents
				std::filesystem::create_directories(destination); // Create the target folder
				for (const auto& entry_t : std::filesystem::recursive_directory_iterator(source))
				{
					const auto& src_path	  = entry_t.path();
					auto		relative_path = std::filesystem::relative(src_path, source);
					auto		dest_path	  = destination / relative_path;

					if (std::filesystem::is_directory(src_path))
						std::filesystem::create_directory(dest_path);
					else if (std::filesystem::is_regular_file(src_path))
						std::filesystem::copy_file(src_path, dest_path, std::filesystem::copy_options::overwrite_existing);
				}
			}
			catch (const std::exception& ex)
			{
				SFG_ERR("Error while copying directory {0}", ex.what());
			}
		}

		void copy_file_to_directory(const char* file, const char* targetParentFolder)
		{
			try
			{
				std::filesystem::path src_file = file;
				std::filesystem::path dest_dir = targetParentFolder;

				// Ensure source file exists
				if (!std::filesystem::exists(src_file) || !std::filesystem::is_regular_file(src_file))
				{
					SFG_ERR("Error: Source file does not exist or is not a valid file. {0} {1}", file, targetParentFolder);
					return;
				}

				// Ensure target directory exists
				if (!std::filesystem::exists(dest_dir))
				{
					SFG_ERR("Error: Target directory does not exist. {0} {1}", file, targetParentFolder);
					return;
				}

				if (!std::filesystem::is_directory(dest_dir))
				{
					SFG_ERR("Error: Target path is not a directory. {0} {1}", file, targetParentFolder);
					return;
				}

				// Construct destination path
				std::filesystem::path destinationFile = dest_dir / src_file.filename();

				// Copy file
				std::filesystem::copy_file(src_file, destinationFile, std::filesystem::copy_options::overwrite_existing);
			}
			catch (const std::filesystem::filesystem_error& e)
			{
				SFG_ERR("Filesystem error : {0} { 1 }", file, targetParentFolder);
			}
		}
	}

}
