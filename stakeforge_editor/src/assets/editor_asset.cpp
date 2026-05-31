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

#include "assets/editor_asset.hpp"

#include "assets/editor_asset_manager.hpp"

#include <sfg/common/hashing.hpp>
#include <sfg/data/string.hpp>
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/resources/shader_types.hpp>
#include <sfg/runtime/resources/texture_cook.hpp>
#include <sfg/runtime/resources/texture_sampler_cook.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>
#include <cstddef>
#include <cstdio>
#include <iterator>

namespace sfg
{
	namespace
	{
#define TEXTURE_COOK_CONFIG_TYPE_ID "texture_cook_config"_hs

		void destroy_texture_cook_config(void* object)
		{
			delete static_cast<texture_cook_config_t*>(object);
		}

		void register_texture_cook_config_reflection()
		{
			reflection_registry_t& registry = reflection_registry_t::get();
			if (registry.find_type(TEXTURE_COOK_CONFIG_TYPE_ID) != nullptr)
				return;

			static const reflected_enum_value_desc_t payload_type_values[] = {
				{.name = "uncompressed", .display_name = "Uncompressed", .value = static_cast<i64>(texture_cook_payload_type_e::uncompressed)},
				{.name = "ktx2_uastc", .display_name = "KTX2 UASTC", .value = static_cast<i64>(texture_cook_payload_type_e::ktx2_uastc)},
			};

			static const reflected_field_desc_t fields[] = {
				{.enum_values  = {.data = payload_type_values, .size = std::size(payload_type_values)},
				 .name		   = "payload_type",
				 .display_name = "Payload Type",
				 .type		   = reflected_value_type_e::enum32,
				 .offset	   = offsetof(texture_cook_config_t, payload_type),
				 .size		   = sizeof(texture_cook_payload_type_e)},
				{.name = "generate_mipmaps", .display_name = "Generate Mipmaps", .type = reflected_value_type_e::bool8, .offset = offsetof(texture_cook_config_t, generate_mipmaps), .size = sizeof(bool)},
				{.name = "is_linear", .display_name = "Linear", .type = reflected_value_type_e::bool8, .offset = offsetof(texture_cook_config_t, is_linear), .size = sizeof(bool)},
			};

			registry.register_type({
				.fields		  = {.data = fields, .size = std::size(fields)},
				.name		  = "texture_cook_config",
				.display_name = "Texture Cook Config",
				.type_id	  = TEXTURE_COOK_CONFIG_TYPE_ID,
				.size		  = sizeof(texture_cook_config_t),
				.alignment	  = alignof(texture_cook_config_t),
			});
		}
	}

	bool editor_asset_loader_audio_t::create_default(editor_asset_t&, const char*, const char*, u8, void*)
	{
		return true;
	}

	void editor_asset_loader_audio_t::register_type()
	{
		editor_asset_manager_t::get().register_descriptor({.create_default = create_default, .extension = "mp3", .asset_type = editor_asset_type_e::audio, .source_type = editor_asset_source_type_e::file});
	}

	bool editor_asset_loader_font_t::create_default(editor_asset_t&, const char*, const char*, u8, void*)
	{
		return true;
	}

	void editor_asset_loader_font_t::register_type()
	{
		editor_asset_manager_t::get().register_descriptor({.create_default = create_default, .extension = "ttf", .asset_type = editor_asset_type_e::font, .source_type = editor_asset_source_type_e::file});
	}

	bool editor_asset_loader_mesh_t::create_default(editor_asset_t&, const char*, const char*, u8, void*)
	{
		return true;
	}

	void editor_asset_loader_mesh_t::register_type()
	{
		editor_asset_manager_t::get().register_descriptor({.create_default = create_default, .extension = "glb", .asset_type = editor_asset_type_e::mesh, .source_type = editor_asset_source_type_e::file});
	}

	bool editor_asset_loader_skeleton_t::create_default(editor_asset_t&, const char*, const char*, u8, void*)
	{
		return true;
	}

	void editor_asset_loader_skeleton_t::register_type()
	{
		editor_asset_manager_t::get().register_descriptor({.create_default = create_default, .asset_type = editor_asset_type_e::skeleton, .source_type = editor_asset_source_type_e::file});
	}

	bool editor_asset_loader_animation_t::create_default(editor_asset_t&, const char*, const char*, u8, void*)
	{
		return true;
	}

	void editor_asset_loader_animation_t::register_type()
	{
		editor_asset_manager_t::get().register_descriptor({.create_default = create_default, .asset_type = editor_asset_type_e::animation, .source_type = editor_asset_source_type_e::file});
	}

	bool editor_asset_loader_material_t::create_default(editor_asset_t&, const char*, const char*, u8, void*)
	{
		return true;
	}

	void editor_asset_loader_material_t::register_type()
	{
		editor_asset_manager_t::get().register_descriptor({.create_default = create_default, .asset_type = editor_asset_type_e::material, .source_type = editor_asset_source_type_e::none});
	}

	bool editor_asset_loader_shader_t::create_default(editor_asset_t& asset, const char*, const char*, u8 sub_type, void*)
	{
		const shader_type_e shader_type = static_cast<shader_type_e>(sub_type);
		asset.cook_options["schema"]	= "sfg.schema.shader";
		switch (shader_type)
		{
		case shader_type_e::opaque_shader:
			asset.cook_options["type"] = "opaque_shader";
			break;
		case shader_type_e::transparent_shader:
			asset.cook_options["type"] = "transparent_shader";
			break;
		case shader_type_e::post_process_shader:
			asset.cook_options["type"] = "post_process_shader";
			break;
		case shader_type_e::ui_shader:
			asset.cook_options["type"] = "ui_shader";
			break;
		case shader_type_e::ui_text_shader:
			asset.cook_options["type"] = "ui_text_shader";
			break;
		default:
			asset.cook_options["type"] = "opaque_shader";
			break;
		}
		return true;
	}

	void editor_asset_loader_shader_t::register_type()
	{
		editor_asset_manager_t::get().register_descriptor({.create_default = create_default, .asset_type = editor_asset_type_e::shader, .source_type = editor_asset_source_type_e::none});
	}

	bool editor_asset_loader_texture_t::create_default(editor_asset_t& asset, const char*, const char*, u8, void* cook_config)
	{
		const texture_cook_config_t texture_config = cook_config != nullptr ? *reinterpret_cast<const texture_cook_config_t*>(cook_config) : texture_cook_config_t{};
		const nlohmann::json		json_data	   = texture_config;
		asset.cook_options						   = json_data;
		return true;
	}

	editor_asset_cook_config_desc_t editor_asset_loader_texture_t::create_cook_config()
	{
		return {.object = new texture_cook_config_t(), .title = "Texture", .destroy = destroy_texture_cook_config, .type_id = TEXTURE_COOK_CONFIG_TYPE_ID};
	}

	void editor_asset_loader_texture_t::register_type()
	{
		register_texture_cook_config_reflection();
		editor_asset_manager_t::get().register_descriptor({.create_default = create_default, .create_cook_config = create_cook_config, .extension = "png;jpg;jpeg", .asset_type = editor_asset_type_e::texture, .source_type = editor_asset_source_type_e::file});
	}

	bool editor_asset_loader_texture_sampler_t::create_default(editor_asset_t& asset, const char*, const char*, u8, void*)
	{
		const sampler_desc_t sampler_desc = {};
		const nlohmann::json json_data	  = sampler_desc;
		asset.embedded_source			  = json_data;
		return true;
	}

	bool editor_asset_loader_texture_sampler_t::cook(const editor_asset_t& asset, ostream_t& stream)
	{
		return texture_sampler_cooker::cook_from_json(asset.embedded_source, stream);
	}

	void editor_asset_loader_texture_sampler_t::register_type()
	{
		editor_asset_manager_t::get().register_descriptor({.create_default = create_default, .cook = cook, .asset_type = editor_asset_type_e::texture_sampler, .source_type = editor_asset_source_type_e::embedded});
	}

	bool editor_asset_loader_physical_material_t::create_default(editor_asset_t&, const char*, const char*, u8, void*)
	{
		return true;
	}

	void editor_asset_loader_physical_material_t::register_type()
	{
		editor_asset_manager_t::get().register_descriptor({.create_default = create_default, .asset_type = editor_asset_type_e::physical_material, .source_type = editor_asset_source_type_e::none});
	}

	bool editor_asset_loader_prefab_t::create_default(editor_asset_t&, const char*, const char*, u8, void*)
	{
		return true;
	}

	void editor_asset_loader_prefab_t::register_type()
	{
		editor_asset_manager_t::get().register_descriptor({.create_default = create_default, .asset_type = editor_asset_type_e::prefab, .source_type = editor_asset_source_type_e::file});
	}

	bool editor_asset_loader_animation_state_machine_t::create_default(editor_asset_t&, const char*, const char*, u8, void*)
	{
		return true;
	}

	void editor_asset_loader_animation_state_machine_t::register_type()
	{
		editor_asset_manager_t::get().register_descriptor({.create_default = create_default, .asset_type = editor_asset_type_e::animation_state_machine, .source_type = editor_asset_source_type_e::none});
	}

	void to_json(nlohmann::json& j, const editor_asset_type_e& t)
	{
		switch (t)
		{
		case editor_asset_type_e::audio:
			j = "audio";
			break;
		case editor_asset_type_e::font:
			j = "font";
			break;
		case editor_asset_type_e::mesh:
			j = "mesh";
			break;
		case editor_asset_type_e::skeleton:
			j = "skeleton";
			break;
		case editor_asset_type_e::animation:
			j = "animation";
			break;
		case editor_asset_type_e::material:
			j = "material";
			break;
		case editor_asset_type_e::shader:
			j = "shader";
			break;
		case editor_asset_type_e::texture:
			j = "texture";
			break;
		case editor_asset_type_e::texture_sampler:
			j = "texture_sampler";
			break;
		case editor_asset_type_e::physical_material:
			j = "physical_material";
			break;
		case editor_asset_type_e::prefab:
			j = "prefab";
			break;
		case editor_asset_type_e::animation_state_machine:
			j = "animation_state_machine";
			break;
		default:
			j = "invalid";
			break;
		}
	}

	void from_json(const nlohmann::json& j, editor_asset_type_e& t)
	{
		const string_t s = j.get<string_t>();

		if (s == "audio")
			t = editor_asset_type_e::audio;
		else if (s == "font")
			t = editor_asset_type_e::font;
		else if (s == "mesh")
			t = editor_asset_type_e::mesh;
		else if (s == "skeleton")
			t = editor_asset_type_e::skeleton;
		else if (s == "animation")
			t = editor_asset_type_e::animation;
		else if (s == "material")
			t = editor_asset_type_e::material;
		else if (s == "shader")
			t = editor_asset_type_e::shader;
		else if (s == "texture")
			t = editor_asset_type_e::texture;
		else if (s == "texture_sampler")
			t = editor_asset_type_e::texture_sampler;
		else if (s == "physical_material")
			t = editor_asset_type_e::physical_material;
		else if (s == "prefab")
			t = editor_asset_type_e::prefab;
		else if (s == "animation_state_machine")
			t = editor_asset_type_e::animation_state_machine;
		else
			t = editor_asset_type_e::invalid;
	}

	void to_json(nlohmann::json& j, const editor_asset_source_type_e& t)
	{
		switch (t)
		{
		case editor_asset_source_type_e::none:
			j = "none";
			break;
		case editor_asset_source_type_e::embedded:
			j = "embedded";
			break;
		default:
			j = "file";
			break;
		}
	}

	void from_json(const nlohmann::json& j, editor_asset_source_type_e& t)
	{
		const string_t s = j.get<string_t>();

		if (s == "none")
			t = editor_asset_source_type_e::none;
		else if (s == "embedded")
			t = editor_asset_source_type_e::embedded;
		else
			t = editor_asset_source_type_e::file;
	}

	void to_json(nlohmann::json& j, const editor_asset_t& asset)
	{
		j["version"]		 = asset.version;
		j["guid"]			 = asset.guid;
		j["asset_type"]		 = asset.asset_type;
		j["sub_type"]		 = asset.sub_type;
		j["embedded_source"] = asset.embedded_source;
		j["cook_options"]	 = asset.cook_options;
	}

	void from_json(const nlohmann::json& j, editor_asset_t& asset)
	{
		asset.version		  = j.value<u32>("version", editor_asset_t::VERSION);
		asset.guid			  = j.value<sid_t>("guid", 0);
		asset.asset_type	  = j.value<editor_asset_type_e>("asset_type", j.value<editor_asset_type_e>("resource_type", j.value<editor_asset_type_e>("type", editor_asset_type_e::invalid)));
		asset.sub_type		  = j.value<u8>("sub_type", 0);
		asset.embedded_source = j.value<nlohmann::json>("embedded_source", nlohmann::json::object());
		asset.cook_options	  = j.value<nlohmann::json>("cook_options", nlohmann::json::object());
	}
}
