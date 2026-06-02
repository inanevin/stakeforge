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

#include "assets/editor_asset_types.hpp"

#include "assets/editor_asset_manager.hpp"
#include "editor_project.hpp"

#include <sfg/common/hashing.hpp>
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/gfx/common/descriptions_reflection.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/math/color.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/resources/material_cook.hpp>
#include <sfg/runtime/resources/physical_material_cook.hpp>
#include <sfg/runtime/resources/shader_cook.hpp>
#include <sfg/runtime/resources/shader_cook_reflection.hpp>
#include <sfg/runtime/resources/shader_types.hpp>
#include <sfg/runtime/resources/texture_cook.hpp>
#include <sfg/runtime/resources/texture_cook_reflection.hpp>
#include <sfg/runtime/resources/texture_sampler_cook.hpp>
#include <cstddef>
#include <iterator>

namespace sfg
{
#define EDITOR_ASSET_COLOR(R, G, B) color_t::from255(R, G, B, 255.0f).srgb_to_linear().to_vector()

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
				{.name = "uncompressed", .display_name = "Uncompressed", .value = static_cast<i64>(texture_payload_type_e::uncompressed)},
				{.name = "ktx2_uastc", .display_name = "KTX2 UASTC", .value = static_cast<i64>(texture_payload_type_e::ktx2_uastc)},
			};

			static const reflected_field_desc_t fields[] = {
				{.enum_values  = {.data = payload_type_values, .size = std::size(payload_type_values)},
				 .name		   = "payload_type",
				 .display_name = "Payload Type",
				 .type		   = reflected_value_type_e::enum32,
				 .offset	   = offsetof(texture_cook_config_t, payload_type),
				 .size		   = sizeof(texture_payload_type_e)},
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

		const char* get_shader_scaffold_relative(shader_type_e type)
		{
			switch (type)
			{
			case shader_type_e::opaque_shader:
				return "editor_scaffold/shaders/world/gbuffer_lit.hlsl";
			case shader_type_e::transparent_shader:
				return "editor_scaffold/shaders/world/forward.hlsl";
			case shader_type_e::post_process_shader:
				return "editor_scaffold/shaders/world/forward.hlsl";
			case shader_type_e::ui_shader:
				return "editor_scaffold/shaders/world/forward.hlsl";
			case shader_type_e::ui_text_shader:
				return "editor_scaffold/shaders/world/forward.hlsl";
			default:
				return "editor_scaffold/shaders/world/gbuffer_lit.hlsl";
			}
		}

		const char* get_material_scaffold_relative(editor_material_type_e type)
		{
			switch (type)
			{
			case editor_material_type_e::forward:
				return "editor_scaffold/materials/material_forward.sfg_asset";
			default:
				return "editor_scaffold/materials/material_gbuffer.sfg_asset";
			}
		}
	}

	bool editor_asset_loader_audio_t::create_default(editor_asset_t&, const char*, const char*, void*)
	{
		return true;
	}

	void editor_asset_loader_audio_t::register_type()
	{
		editor_asset_manager_t::get().register_descriptor({.create_default = create_default, .extensions = {"mp3"}, .display_name = "Audio", .color = EDITOR_ASSET_COLOR(64.0f, 177.0f, 255.0f), .asset_type = editor_asset_type_e::audio});
	}

	bool editor_asset_loader_font_t::create_default(editor_asset_t&, const char*, const char*, void*)
	{
		return true;
	}

	void editor_asset_loader_font_t::register_type()
	{
		editor_asset_manager_t::get().register_descriptor({.create_default = create_default, .extensions = {"ttf"}, .display_name = "Font", .color = EDITOR_ASSET_COLOR(245.0f, 194.0f, 82.0f), .asset_type = editor_asset_type_e::font});
	}

	bool editor_asset_loader_mesh_t::create_default(editor_asset_t&, const char*, const char*, void*)
	{
		return true;
	}

	void editor_asset_loader_mesh_t::register_type()
	{
		editor_asset_manager_t::get().register_descriptor({.create_default = create_default, .extensions = {"glb"}, .display_name = "Mesh", .color = EDITOR_ASSET_COLOR(158.0f, 120.0f, 255.0f), .asset_type = editor_asset_type_e::mesh});
	}

	bool editor_asset_loader_skeleton_t::create_default(editor_asset_t&, const char*, const char*, void*)
	{
		return true;
	}

	void editor_asset_loader_skeleton_t::register_type()
	{
		editor_asset_manager_t::get().register_descriptor({.create_default = create_default, .display_name = "Skeleton", .color = EDITOR_ASSET_COLOR(184.0f, 155.0f, 255.0f), .asset_type = editor_asset_type_e::skeleton});
	}

	bool editor_asset_loader_animation_t::create_default(editor_asset_t&, const char*, const char*, void*)
	{
		return true;
	}

	void editor_asset_loader_animation_t::register_type()
	{
		editor_asset_manager_t::get().register_descriptor({.create_default = create_default, .display_name = "Animation", .color = EDITOR_ASSET_COLOR(255.0f, 129.0f, 80.0f), .asset_type = editor_asset_type_e::animation});
	}

	bool editor_asset_loader_material_t::create_default(editor_asset_t& asset, const char*, const char*, void*)
	{
		const editor_material_type_e material_type = static_cast<editor_material_type_e>(asset.sub_type);
		string_t					 scaffold_path = file_system_t::get_running_directory();
		file_system_t::fix_path_end_slash(scaffold_path);
		scaffold_path += get_material_scaffold_relative(material_type);
		SFG_ASSERT(file_system_t::exists(scaffold_path.c_str()));

		editor_asset_t scaffold_asset = {};
		const bool	   read_result	  = editor_asset_util_t::read_asset(scaffold_path.c_str(), scaffold_asset);
		SFG_ASSERT(read_result);

		asset.source_type	  = editor_asset_source_type_e::embedded;
		asset.embedded_source = scaffold_asset.embedded_source;
		return true;
	}

	bool editor_asset_loader_material_t::cook(const editor_asset_t& asset, ostream_t& stream)
	{
		return material_cooker::cook_from_json(asset.embedded_source, stream);
	}

	void editor_asset_loader_material_t::register_type()
	{
		editor_asset_manager_t::get().register_descriptor({.create_default = create_default, .cook = cook, .display_name = "Material", .color = EDITOR_ASSET_COLOR(255.0f, 102.0f, 0.0f), .asset_type = editor_asset_type_e::material});
	}

	bool editor_asset_loader_shader_t::create_default(editor_asset_t& asset, const char* directory, const char* file_name, void*)
	{
		const shader_type_e shader_type	   = static_cast<shader_type_e>(asset.sub_type);
		asset.cook_options["schema"]	   = "sfg.schema.shader";
		asset.cook_options["include_dirs"] = {"editor_scaffold/shaders", "editor_scaffold/shaders/world"};
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

		if (asset.source_relative.empty())
		{
			SFG_ASSERT(directory != nullptr);
			SFG_ASSERT(directory[0] != '\0');
			SFG_ASSERT(file_name != nullptr);
			SFG_ASSERT(file_name[0] != '\0');

			string_t scaffold_path = file_system_t::get_running_directory();
			file_system_t::fix_path_end_slash(scaffold_path);
			scaffold_path += get_shader_scaffold_relative(shader_type);
			SFG_ASSERT(file_system_t::exists(scaffold_path.c_str()));

			const string_t source_path = editor_asset_util_t::make_unique_source_path(directory, file_name, "hlsl");
			if (!file_system_t::copy_file(scaffold_path.c_str(), source_path.c_str()))
				return false;

			SFG_ASSERT(file_system_t::exists(source_path.c_str()));
			asset.source_relative = editor_asset_util_t::get_source_relative(editor_project_t::get()._runtime.assets_path.c_str(), source_path.c_str());
			SFG_ASSERT(!asset.source_relative.empty());
		}
		return true;
	}

	bool editor_asset_loader_shader_t::cook(const editor_asset_t& asset, ostream_t& stream)
	{
		const string_t			   source_full_path = editor_asset_util_t::get_source_full_path(editor_project_t::get()._runtime.assets_path.c_str(), asset);
		const shader_cook_config_t config			= asset.cook_options;
		return shader_cooker::cook_from_file(config, source_full_path.c_str(), stream);
	}

	void editor_asset_loader_shader_t::register_type()
	{
		editor_asset_manager_t::get().register_descriptor({.create_default = create_default, .cook = cook, .extensions = {"hlsl"}, .display_name = "Shader", .color = EDITOR_ASSET_COLOR(90.0f, 190.0f, 255.0f), .asset_type = editor_asset_type_e::shader});
	}

	bool editor_asset_loader_texture_t::create_default(editor_asset_t& asset, const char*, const char*, void* cook_config)
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

	bool editor_asset_loader_texture_t::cook(const editor_asset_t& asset, ostream_t& stream)
	{
		const texture_cook_config_t config = asset.cook_options;
		if (asset.source_type == editor_asset_source_type_e::data)
		{
			SFG_ASSERT(asset._transient_data.data != nullptr);
			SFG_ASSERT(asset._transient_data.size != 0);
			const bool result = texture_cooker::cook_from_data(config, asset._transient_data, stream);
			SFG_FREE(asset._transient_data.data);
			return result;
		}

		const string_t source_full_path = editor_asset_util_t::get_source_full_path(editor_project_t::get()._runtime.assets_path.c_str(), asset);
		return texture_cooker::cook_from_file(config, source_full_path.c_str(), stream);
	}

	void editor_asset_loader_texture_t::register_type()
	{
		register_texture_cook_config_reflection();
		editor_asset_manager_t::get().register_descriptor({.create_default	   = create_default,
														   .create_cook_config = create_cook_config,
														   .cook			   = cook,
														   .extensions		   = {"png", "jpg", "jpeg"},
														   .display_name	   = "Texture",
														   .color			   = EDITOR_ASSET_COLOR(151.0f, 0.0f, 119.0f),
														   .asset_type		   = editor_asset_type_e::texture});
	}

	bool editor_asset_loader_texture_sampler_t::create_default(editor_asset_t& asset, const char*, const char*, void*)
	{
		const sampler_desc_t sampler_desc = {};
		const nlohmann::json json_data	  = sampler_desc;
		asset.source_type				  = editor_asset_source_type_e::embedded;
		asset.embedded_source			  = json_data;
		return true;
	}

	bool editor_asset_loader_texture_sampler_t::cook(const editor_asset_t& asset, ostream_t& stream)
	{
		return texture_sampler_cooker::cook_from_json(asset.embedded_source, stream);
	}

	void editor_asset_loader_texture_sampler_t::register_type()
	{
		editor_asset_manager_t::get().register_descriptor({.create_default = create_default, .cook = cook, .display_name = "Texture Sampler", .color = EDITOR_ASSET_COLOR(180.0f, 0.0f, 119.0f), .asset_type = editor_asset_type_e::texture_sampler});
	}

	bool editor_asset_loader_physical_material_t::create_default(editor_asset_t& asset, const char*, const char*, void*)
	{
		string_t scaffold_path = file_system_t::get_running_directory();
		file_system_t::fix_path_end_slash(scaffold_path);
		scaffold_path += "editor_scaffold/materials/physical_material.sfg_asset";
		SFG_ASSERT(file_system_t::exists(scaffold_path.c_str()));

		editor_asset_t scaffold_asset = {};
		const bool	   read_result	  = editor_asset_util_t::read_asset(scaffold_path.c_str(), scaffold_asset);
		SFG_ASSERT(read_result);

		asset.source_type	  = editor_asset_source_type_e::embedded;
		asset.embedded_source = scaffold_asset.embedded_source;
		return true;
	}

	bool editor_asset_loader_physical_material_t::cook(const editor_asset_t& asset, ostream_t& stream)
	{
		return physical_material_cooker::cook_from_json(asset.embedded_source, stream);
	}

	void editor_asset_loader_physical_material_t::register_type()
	{
		editor_asset_manager_t::get().register_descriptor({.create_default = create_default, .cook = cook, .display_name = "Physical Material", .color = EDITOR_ASSET_COLOR(214.0f, 65.0f, 57.0f), .asset_type = editor_asset_type_e::physical_material});
	}

	bool editor_asset_loader_prefab_t::create_default(editor_asset_t&, const char*, const char*, void*)
	{
		return true;
	}

	void editor_asset_loader_prefab_t::register_type()
	{
		editor_asset_manager_t::get().register_descriptor({.create_default = create_default, .display_name = "Prefab", .color = EDITOR_ASSET_COLOR(107.0f, 210.0f, 132.0f), .asset_type = editor_asset_type_e::prefab});
	}

	bool editor_asset_loader_animation_state_machine_t::create_default(editor_asset_t& asset, const char*, const char*, void*)
	{
		asset.source_type = editor_asset_source_type_e::none;
		return true;
	}

	void editor_asset_loader_animation_state_machine_t::register_type()
	{
		editor_asset_manager_t::get().register_descriptor({.create_default = create_default, .display_name = "State Machine", .color = EDITOR_ASSET_COLOR(245.0f, 118.0f, 182.0f), .asset_type = editor_asset_type_e::animation_state_machine});
	}
}
