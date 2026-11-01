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
#include "editor/editor_settings.hpp"
#include "editor/editor_directories.hpp"

#include <sfg/core/io/file_system.hpp>
#include <sfg/core/io/log.hpp>
#include <sfg/core/math/math.hpp>
#include <sfg/core/reflection/reflection_registry.hpp>
#include <sfg/core/serialization/serialization.hpp>
#include <sfg/core/vendor/nhlohmann/json.hpp>

namespace sfg
{
	editor_settings_reflection_t g_reflect_editor_settings = {};

	namespace
	{
		template <typename T> nlohmann::json reflected_to_json(const T& value)
		{
			nlohmann::json out = nlohmann::json::object();

			if (!reflection_registry_t::get().type_to_json(type_id_t<T>::value, const_cast<T*>(&value), nullptr, out))
				throw nlohmann::json::type_error::create(302, "failed to serialize reflected editor settings", &out);

			return out;
		}

		template <typename T> void reflected_from_json(const nlohmann::json& json, T& value)
		{
			if (!json.is_object() || !reflection_registry_t::get().type_from_json(type_id_t<T>::value, &value, nullptr, json))
				throw nlohmann::json::type_error::create(302, "failed to deserialize reflected editor settings", &json);
		}
	}

	void editor_settings_configurable_t::normalize()
	{
		editor_ui_scale = math::clamp(editor_ui_scale, 0.1f, 8.0f);
	}

	bool editor_settings_t::save()
	{
		const string_t settings = editor_directories_t::get_editor_settings();
		string_t	   data		= {};

		try
		{
			const nlohmann::json json_data = *this;

			data = json_data.dump(4);
		}
		catch (const nlohmann::json::exception& error)
		{
			SFG_ERR("failed to serialize editor settings {0}: {1}", settings, error.what());
			return false;
		}

		if (!serializer_t::write_to_file(string_view_t(data.data(), data.size()), settings.c_str()))
		{
			SFG_ERR("failed to write editor settings {0}", settings);
			return false;
		}

		return true;
	}

	bool editor_settings_t::ensure_loaded()
	{
		const string_t settings = editor_directories_t::get_editor_settings();

		if (!file_system_t::exists(settings.c_str()))
		{
			editor_settings_t defaults = {};

			if (!defaults.save())
				return false;

			*this = std::move(defaults);

			return true;
		}

		const string_t		 json_text = file_system_t::read_file_as_string(settings.c_str());
		const nlohmann::json json	   = nlohmann::json::parse(json_text, nullptr, false);

		if (json.is_discarded())
		{
			SFG_ERR("failed to parse editor settings {0}", settings);
			return false;
		}

		editor_settings_t loaded = {};

		try
		{
			json.get_to(loaded);
		}
		catch (const nlohmann::json::exception& error)
		{
			SFG_ERR("failed to deserialize editor settings {0}: {1}", settings, error.what());
			return false;
		}

		for (u8 i = 0; i < static_cast<u8>(editor_panel_type_e::max); ++i)
		{
			const editor_panel_window_size_t& size	= loaded.panel_window_sizes[i];
			const bool						  unset = size.width == 0.0f && size.height == 0.0f;
			const bool						  valid = size.width > 0.0f && size.width <= 65535.0f && size.height > 0.0f && size.height <= 65535.0f;

			if (!unset && !valid)
			{
				SFG_ERR("invalid window size for panel {0} in editor settings {1}", editor_panel_type_to_string(static_cast<editor_panel_type_e>(i)), settings);
				return false;
			}
		}

		*this = std::move(loaded);

		return true;
	}

}

namespace sfg
{
	editor_settings_reflection_t::editor_settings_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name		  = "editor_settings_configurable_t",
			.display_name = "Editor",
			.fields =
				{
					{.name				= "editor_ui_scale",
					 .display_name		= "Editor UI Scale",
					 .offset			= offsetof(editor_settings_configurable_t, editor_ui_scale),
					 .size				= sizeof(f32),
					 .flags				= reflected_field_flag_clamped,
					 .min_clamp			= 0.1f,
					 .max_clamp			= 8.0f,
					 .clamp_granularity = 0.1f,
					 .type				= reflected_value_type_e::f32},
				},
			.type_id   = type_id_t<editor_settings_configurable_t>::value,
			.size	   = sizeof(editor_settings_configurable_t),
			.alignment = alignof(editor_settings_configurable_t),
		});
	}

	void to_json(nlohmann::json& j, const editor_import_settings_t& settings)
	{
		j["texture"] = reflected_to_json(settings.texture);
		j["audio"]	 = reflected_to_json(settings.audio);
		j["cubemap"] = reflected_to_json(settings.cubemap);
		j["glb"]	 = reflected_to_json(settings.glb);
		j["sprite"]	 = reflected_to_json(settings.sprite);
	}

	void from_json(const nlohmann::json& j, editor_import_settings_t& settings)
	{
		reflected_from_json(j.value("texture", nlohmann::json::object()), settings.texture);
		reflected_from_json(j.value("audio", nlohmann::json::object()), settings.audio);
		reflected_from_json(j.value("cubemap", nlohmann::json::object()), settings.cubemap);
		reflected_from_json(j.value("glb", nlohmann::json::object()), settings.glb);
		reflected_from_json(j.value("sprite", nlohmann::json::object()), settings.sprite);
	}

	void to_json(nlohmann::json& j, const editor_panel_window_size_t& size)
	{
		j["width"]	= size.width;
		j["height"] = size.height;
	}

	void from_json(const nlohmann::json& j, editor_panel_window_size_t& size)
	{
		size.width	= j.value<f32>("width", 0.0f);
		size.height = j.value<f32>("height", 0.0f);
	}

	void to_json(nlohmann::json& j, const editor_settings_t& settings)
	{
		j["import"]				= settings.import;
		j["project_cook"]		= reflected_to_json(settings.project_cook);
		j["configurable"]		= reflected_to_json(settings.configurable);
		j["project_path"]		= settings.last_project_path;
		j["panel_window_sizes"] = nlohmann::json::object();

		for (u8 i = 0; i < static_cast<u8>(editor_panel_type_e::max); ++i)
			j["panel_window_sizes"][editor_panel_type_to_string(static_cast<editor_panel_type_e>(i))] = settings.panel_window_sizes[i];
	}

	void from_json(const nlohmann::json& j, editor_settings_t& settings)
	{
		settings.import = j.value("import", editor_import_settings_t{});
		reflected_from_json(j.value("project_cook", nlohmann::json::object()), settings.project_cook);
		settings.configurable = {};
		reflected_from_json(j.value("configurable", nlohmann::json::object()), settings.configurable);
		settings.configurable.normalize();
		settings.last_project_path = j.value<string_t>("project_path", {});

		const nlohmann::json panel_window_sizes = j.value("panel_window_sizes", nlohmann::json::object());

		for (u8 i = 0; i < static_cast<u8>(editor_panel_type_e::max); ++i)
			settings.panel_window_sizes[i] = panel_window_sizes.value<editor_panel_window_size_t>(editor_panel_type_to_string(static_cast<editor_panel_type_e>(i)), {});
	}
}
