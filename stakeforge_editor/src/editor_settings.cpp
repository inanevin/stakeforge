// Copyright (c) 2025 Inan Evin

#include "editor_settings.hpp"
#include "editor_directories.hpp"

#include <sfg/common/size_definitions.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/serialization/serialization.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	void to_json(nlohmann::json& j, const editor_window_settings_t& settings)
	{
		j["position"]			= nlohmann::json::array_t({settings.position.x, settings.position.y});
		j["size"]				= settings.size;
		j["monitor_identifier"] = settings.monitor_ident;
	}

	void from_json(const nlohmann::json& j, editor_window_settings_t& settings)
	{
		const nlohmann::json position = j.value("position", nlohmann::json::array_t({64, 64}));
		if (position.is_array() && position.size() >= 2)
			settings.position = {position.at(0).get<i16>(), position.at(1).get<i16>()};

		settings.size		   = j.value("size", vec2u16_t{1280, 720});
		settings.monitor_ident = j.value<u64>("monitor_identifier", UINT64_MAX);
	}

	void to_json(nlohmann::json& j, const editor_settings_t& settings)
	{
		j["windows"] = settings._windows;
	}

	void from_json(const nlohmann::json& j, editor_settings_t& settings)
	{
		settings._windows = j.value("windows", vector_t<editor_window_settings_t>{});
	}

	bool editor_settings_t::reload()
	{
		const string_t directory = editor_directories_t::get_user_directory();
		if (!file_system_t::exists(directory.c_str()) && !file_system_t::create_directory(directory.c_str()))
			return false;

		const string_t path = editor_directories_t::get_settings_path();
		if (!file_system_t::exists(path.c_str()))
		{
			_windows.resize(0);
			_windows.push_back({});
			flush_to_disk();
			return true;
		}

		try
		{
			const string_t data = file_system_t::read_file_as_string(path.c_str());
			nlohmann::json::parse(data).get_to(*this);
		}
		catch (const std::exception& e)
		{
			SFG_ERR("failed loading editor settings: {0}", e.what());
			_windows.resize(0);
			flush_to_disk();
			return true;
		}

		if (_windows.empty())
			_windows.push_back({});

		return true;
	}

	void editor_settings_t::save()
	{
		flush_to_disk();
	}

	u16 editor_settings_t::add_window(const editor_window_settings_t& w)
	{
		const u16 idx = static_cast<u16>(_windows.size());
		_windows.push_back(w);
		return idx;
	}

	void editor_settings_t::remove_window(u16 index)
	{
		_windows.erase(_windows.begin() + index);
	}

	void editor_settings_t::flush_to_disk()
	{
		const string_t directory = editor_directories_t::get_user_directory();
		if (!file_system_t::exists(directory.c_str()) && !file_system_t::create_directory(directory.c_str()))
			return;

		const nlohmann::json json_data = *this;
		const string_t		 data	   = json_data.dump(4);
		serializer_t::write_to_file(string_view_t(data.data(), data.size()), editor_directories_t::get_settings_path().c_str());
	}
}
