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
#include "editor_settings.hpp"
#include "editor_directories.hpp"

#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/math.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/serialization/serialization.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	namespace
	{
		template <typename T> nlohmann::json reflected_to_json(const T& value)
		{
			nlohmann::json out = nlohmann::json::object();
			reflection_registry_t::get().type_to_json(type_id_t<T>::value, const_cast<T*>(&value), nullptr, out);
			return out;
		}

		template <typename T> void reflected_from_json(const nlohmann::json& json, T& value)
		{
			reflection_registry_t::get().type_from_json(type_id_t<T>::value, &value, nullptr, json);
		}
	}

	void editor_settings_configurable_t::normalize()
	{
		editor_ui_scale = math::clamp(editor_ui_scale, 0.1f, 8.0f);
	}

	bool editor_settings_t::save()
	{
		const nlohmann::json json_data = *this;

		const string_t data		= json_data.dump(4);
		const string_t settings = editor_directories_t::get_editor_settings();
		if (!serializer_t::write_to_file(string_view_t(data.data(), data.size()), settings.c_str()))
		{
			SFG_ERR("failed file serialization!");
			return false;
		}

		return true;
	}

	bool editor_settings_t::ensure_loaded()
	{
		const string_t settings = editor_directories_t::get_editor_settings();
		if (!file_system_t::exists(settings.c_str()))
		{
			editor_settings_t::get() = {};
			return editor_settings_t::get().save();
		}

		const string_t		 json_text = file_system_t::read_file_as_string(settings.c_str());
		const nlohmann::json json	   = nlohmann::json::parse(json_text, nullptr, false);
		if (json.is_discarded())
			return false;

		*this = json;
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

	void to_json(nlohmann::json& j, const editor_settings_t& settings)
	{
		j["layout"]		  = settings.layout;
		j["import"]		  = settings.import;
		j["project_cook"] = reflected_to_json(settings.project_cook);
		j["configurable"] = reflected_to_json(settings.configurable);
		j["project_path"] = settings.last_project_path;
	}

	void from_json(const nlohmann::json& j, editor_settings_t& settings)
	{
		settings.layout = j.value("layout", editor_layout_t{});
		settings.import = j.value("import", editor_import_settings_t{});
		reflected_from_json(j.value("project_cook", nlohmann::json::object()), settings.project_cook);
		settings.configurable = {};
		reflected_from_json(j.value("configurable", nlohmann::json::object()), settings.configurable);
		settings.configurable.normalize();
		settings.last_project_path = j.value<string_t>("project_path", {});
	}
}
