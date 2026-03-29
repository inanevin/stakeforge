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

#include "editor_settings.hpp"
#include "io/file_system.hpp"
#include "io/log.hpp"
#include "platform/process.hpp"
#include "platform/window.hpp"
#include "math/math.hpp"

#include <fstream>
#include <vendor/nhlohmann/json.hpp>
using json = nlohmann::json;

namespace SFG
{

	void to_json(nlohmann::json& j, const editor_settings& t)
	{
		j["last_world"]	 = t.last_world_relative;
		j["working_dir"] = t.working_dir;
		j["cache_dir"]	 = t.cache_dir;
		j["window_size"] = t.window_size;
		j["window_pos"]	 = t.window_pos;
	}

	void from_json(const nlohmann::json& j, editor_settings& s)
	{
		s.last_world_relative = j.value<string>("last_world", "");
		s.working_dir		  = j.value<string>("working_dir", "");
		s.cache_dir			  = j.value<string>("cache_dir", "");
		s.window_size		  = j.value<vector2>("window_size", vector2(1920, 1080));
		s.window_pos		  = j.value<vector2>("window_pos", vector2(-1, -1));
	}

	bool editor_settings::init()
	{
		_editor_folder = file_system::get_user_directory() + "/stakeforge/";
		file_system::fix_path(_editor_folder);

		if (!file_system::exists(_editor_folder.c_str()))
			file_system::create_directory(_editor_folder.c_str());

		const string last_path = _editor_folder + "editor.stksettings";

		if (file_system::exists(last_path.c_str()))
		{
			load(last_path.c_str());
		}
		else
		{
			// init defaults
			save(last_path.c_str());
		}
		_last_path		= last_path;
		_editor_folder	= file_system::get_user_directory() + "/stakeforge/";
		_resource_cache = editor_settings::get()._editor_folder + "_resource_cache/";
		if (!file_system::exists(_resource_cache.c_str()))
			file_system::create_directory(_resource_cache.c_str());

		if (file_system::exists(working_dir.c_str()))
		{
			if (!file_system::exists(cache_dir.c_str()))
			{
				cache_dir = working_dir + "/_stakeforge_cache/";
				file_system::create_directory(cache_dir.c_str());
				save_last();
			}
		}
		else
		{
			working_dir = process::select_folder("select project directory") + "/";
			if (!file_system::exists(working_dir.c_str()))
				return false;

			file_system::fix_path(working_dir);
			cache_dir = working_dir + "/_stakeforge_cache/";

			if (!file_system::exists(cache_dir.c_str()))
				file_system::create_directory(cache_dir.c_str());
			save_last();
		}

		return true;
	}

	void editor_settings::init_window_layout(window& wnd)
	{
		vector2 ws = editor_settings::get().window_size;
		vector2 wp = editor_settings::get().window_pos;

		const monitor_info&	 window_monitor = wnd.get_monitor_info();
		vector<monitor_info> all_monitors;
		window::query_all_monitors(all_monitors);

		wnd.set_position(vector2i16(wp.x, wp.y));
		wnd.set_size(vector2ui16(ws.x, ws.y));
		// make sure there is a monitor that fits the position.
		// if (math::almost_equal(wp.x, -1.0f) && math::almost_equal(wp.y, -1.0f))
		// {
		// 	wp = vector2(0, 0);
		// 	wnd.set_position(vector2i16(wp.x, wp.y));
		// 	window_pos = wp;
		// 	save_last();
		// }
		// else
		// {
		// 	bool found = false;
		// 	for (const monitor_info& mi : all_monitors)
		// 	{
		// 		if (mi.position.x <= wp.x && mi.position.y <= wp.y && wp.x < mi.position.x + mi.work_size.x && wp.y < mi.position.y + mi.work_size.y)
		// 		{
		// 			found = true;
		// 			break;
		// 		}
		// 	}
		//
		// 	if (found)
		// 	{
		// 		wnd.set_position(vector2i16(wp.x, wp.y));
		// 	}
		// 	else
		// 	{
		// 		wp = vector2(0, 0);
		// 		wnd.set_position(vector2i16(wp.x, wp.y));
		// 		window_pos = wp;
		// 		save_last();
		// 	}
		// }
		//
		// if (ws.x < 1 || ws.y < 1)
		// {
		// 	const monitor_info& mi = wnd.get_monitor_info();
		// 	wnd.set_size(mi.work_size);
		// 	wnd.maximize();
		// 	editor_settings::get().window_size = mi.work_size;
		// 	editor_settings::get().save_last();
		// }
		// else
		// {
		// 	wnd.set_size(vector2ui16(ws.x, ws.y));
		// }

		if (wnd.is_maximized())
			wnd.maximize();
	}

	void editor_settings::uninit()
	{
		this->~editor_settings();
	}

	bool editor_settings::load(const char* path)
	{
		if (!file_system::exists(path))
		{
			SFG_ERR("file doesn't exists: {0}", path);
			return false;
		}

		try
		{
			std::ifstream	f(path);
			editor_settings st = json::parse(f);
			*this			   = st;
			f.close();
		}
		catch (std::exception e)
		{
			SFG_ERR("Failed loading editor_settings: {0}", e.what());
			return false;
		}

		return true;
	}

	bool editor_settings::save(const char* path)
	{
		json j = *this;

		std::ofstream file(path);
		if (file.is_open())
		{
			file << j.dump(4);
			file.close();
			return true;
		}

		SFG_ERR("failed while writing json! {0}", path);
		return false;
	}

	bool editor_settings::is_in_work_directory(const string& path) const
	{
		const size_t found = path.find(working_dir);
		return found != string::npos;
	}

	string editor_settings::get_relative(const string& path) const
	{
		return path.substr(working_dir.size(), path.size() - working_dir.size());
	}

}
