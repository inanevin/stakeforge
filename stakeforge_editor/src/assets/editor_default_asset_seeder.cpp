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

#include "assets/editor_default_asset_seeder.hpp"
#include "assets/editor_asset.hpp"
#include "assets/editor_asset_builtin_types.hpp"
#include "assets/editor_asset_cooker.hpp"
#include "assets/editor_asset_io.hpp"
#include "assets/editor_asset_path.hpp"
#include "assets/editor_asset_util.hpp"
#include "assets/editor_asset_writer.hpp"
#include "editor_project.hpp"

#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/runtime/resources/shader_cook.hpp>
#include <sfg/runtime/resources/shader_types.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
#define COMMON_SHADERS			 "common/shaders/"
#define EDITOR_DEFAULT_MATERIALS "editor_defaults/materials/"
#define EDITOR_DEFAULT_SAMPLERS	 "editor_defaults/samplers/"
#define EDITOR_DEFAULT_TEXTURES	 "editor_defaults/textures/"
#define EDITOR_DEFAULT_SKY		 "editor_defaults/sky/"

	namespace
	{
		struct default_shader_asset_desc_t
		{
			const char*	  asset_name;
			const char*	  source_base_name;
			sid_t		  guid;
			shader_type_e shader_type;
		};

		struct default_file_asset_desc_t
		{
			const char* asset_name;
			const char* source_base_name;
			sid_t		guid;
		};

		struct default_embedded_asset_desc_t
		{
			const char*			asset_name;
			const char*			asset_relative_path;
			sid_t				guid;
			editor_asset_type_e asset_type;
			u8					sub_type;
		};

		bool is_default_asset_ready(const char* default_assets_dir, const char* asset_name, sid_t guid, editor_asset_type_e asset_type, u8 sub_type)
		{
			const string_t asset_path = editor_asset_path_t::make_asset_path(default_assets_dir, asset_name);
			editor_asset_t asset	  = {};

			if (!file_system_t::exists(asset_path.c_str()))
				return false;

			if (!editor_asset_io_t::read_asset(asset_path.c_str(), asset))
				return false;

			if (asset.guid != guid || asset.asset_type != asset_type || asset.sub_type != sub_type)
				return false;

			if (asset.source_type == editor_asset_source_type_e::embedded)
			{
				if (asset.embedded_source.empty())
					return false;
			}
			else if (asset.source_type == editor_asset_source_type_e::file || asset.source_type == editor_asset_source_type_e::file_blob)
			{
				if (asset.source_relative.empty())
					return false;
				if (asset_type == editor_asset_type_e::shader)
				{
					if (asset.embedded_source.empty())
						return false;
					const nlohmann::json embedded_source = editor_asset_io_t::get_embedded_source_json(asset);
					if (!embedded_source.is_object() || !embedded_source.contains("textures") || !embedded_source.contains("samplers") || !embedded_source.contains("parameters"))
						return false;
				}
				const string_t source_path = editor_asset_path_t::get_source_full_path(editor_project_t::get()._runtime.assets_path.c_str(), asset);
				if (!file_system_t::exists(source_path.c_str()))
					return false;
			}

			return editor_asset_cooker_t::is_asset_cooked(asset);
		}

		void build_shader_default_cook_options(shader_type_e shader_type, nlohmann::json& out)
		{
			out["schema"]		= "sfg.schema.shader";
			out["include_dirs"] = {COMMON_SHADERS, COMMON_SHADERS "world/"};
			out["type"]			= static_cast<u8>(shader_type);
		}

		string_t get_shader_default_source_relative(shader_type_e shader_type, const char* source_base_name)
		{
			string_t result = COMMON_SHADERS;

			switch (shader_type)
			{
			case shader_type_e::transparent_shader:
				result += "world/forward.hlsl";
				break;
			case shader_type_e::unlit_shader:
				result += "world/gbuffer_unlit.hlsl";
				break;
			case shader_type_e::skybox_shader:
				result += string_t(source_base_name) == "default_shader_skybox_gradient" ? "world/skybox_gradient.hlsl" : "world/skybox_cube.hlsl";
				break;
			default:
				result += "world/gbuffer_lit.hlsl";
				break;
			}

			return result;
		}

		string_t get_texture_default_asset_relative(const char* texture_name)
		{
			string_t result = EDITOR_DEFAULT_TEXTURES;
			result += texture_name != nullptr ? texture_name : "";
			result += ".sfg_asset";
			return result;
		}

		string_t get_texture_default_source_relative(const char* texture_name)
		{
			string_t result = EDITOR_DEFAULT_TEXTURES;
			result += texture_name != nullptr ? texture_name : "";
			result += ".png";
			return result;
		}

		string_t get_cubemap_default_source_relative(const char* cubemap_name)
		{
			string_t result = EDITOR_DEFAULT_SKY;
			result += cubemap_name != nullptr ? cubemap_name : "";
			result += ".hdr";
			return result;
		}

		string_t get_cubemap_default_asset_relative(const char* cubemap_name)
		{
			string_t result = EDITOR_DEFAULT_SKY;
			result += cubemap_name != nullptr ? cubemap_name : "";
			result += ".sfg_asset";
			return result;
		}

		void ensure_shader_assets(const char* default_assets_dir)
		{
			const default_shader_asset_desc_t default_shader_assets[] = {
				{.asset_name = "default_shader_gbuffer", .source_base_name = "default_shader_gbuffer", .guid = DEFAULT_GBUFFER_SHADER_ASSET_GUID, .shader_type = shader_type_e::opaque_shader},
				{.asset_name = "default_shader_forward", .source_base_name = "default_shader_forward", .guid = DEFAULT_FORWARD_SHADER_ASSET_GUID, .shader_type = shader_type_e::transparent_shader},
				{.asset_name = "default_shader_unlit", .source_base_name = "default_shader_unlit", .guid = DEFAULT_UNLIT_SHADER_ASSET_GUID, .shader_type = shader_type_e::unlit_shader},
				{.asset_name = "default_shader_skybox_cube", .source_base_name = "default_shader_skybox_cube", .guid = DEFAULT_CUBE_SKYBOX_SHADER_ASSET_GUID, .shader_type = shader_type_e::skybox_shader},
				{.asset_name = "default_shader_skybox_gradient", .source_base_name = "default_shader_skybox_gradient", .guid = DEFAULT_GRADIENT_SKYBOX_SHADER_ASSET_GUID, .shader_type = shader_type_e::skybox_shader},
			};

			for (const default_shader_asset_desc_t& desc : default_shader_assets)
			{
				const u8 sub_type = static_cast<u8>(desc.shader_type);
				if (is_default_asset_ready(default_assets_dir, desc.asset_name, desc.guid, editor_asset_type_e::shader, sub_type))
					continue;

				nlohmann::json cook_options = {};
				build_shader_default_cook_options(desc.shader_type, cook_options);
				const string_t source_relative = get_shader_default_source_relative(desc.shader_type, desc.source_base_name);

				const editor_asset_write_file_desc_t write_desc{
					.cook_options			  = &cook_options,
					.parent_path			  = default_assets_dir,
					.name					  = desc.asset_name,
					.source_name			  = desc.source_base_name,
					.source_extension		  = "hlsl",
					.source_template_relative = source_relative.c_str(),
					.guid					  = desc.guid,
					.asset_type				  = editor_asset_type_e::shader,
					.sub_type				  = sub_type,
					.allow_overwrite		  = true,
				};
				editor_asset_t asset	  = {};
				string_t	   asset_path = {};
				bool		   created	  = editor_asset_writer_t::write_file_asset(write_desc, &asset, &asset_path);

				if (created)
				{
					shader_data_definition_t definition = {};
					created								= editor_asset_cooker_t::cook_shader(asset, desc.asset_name, &definition);
					if (created)
					{
						editor_asset_io_t::set_embedded_source_json(asset, definition);
						created = editor_asset_io_t::write_asset(asset_path.c_str(), asset);
					}
				}

				SFG_ASSERT(created);
			}
		}

		void ensure_texture_assets(const char* default_assets_dir)
		{
			const default_file_asset_desc_t default_texture_assets[] = {
				{.asset_name = "default_texture_albedo", .source_base_name = "default_texture_albedo", .guid = DEFAULT_ALBEDO_TEXTURE_ASSET_GUID},
				{.asset_name = "default_texture_orm", .source_base_name = "default_texture_orm", .guid = DEFAULT_ORM_TEXTURE_ASSET_GUID},
				{.asset_name = "default_texture_normal", .source_base_name = "default_texture_normal", .guid = DEFAULT_NORMAL_TEXTURE_ASSET_GUID},
				{.asset_name = "default_texture_emissive", .source_base_name = "default_texture_emissive", .guid = DEFAULT_EMISSIVE_TEXTURE_ASSET_GUID},
			};

			for (const default_file_asset_desc_t& desc : default_texture_assets)
			{
				if (is_default_asset_ready(default_assets_dir, desc.asset_name, desc.guid, editor_asset_type_e::texture, 0))
					continue;

				nlohmann::json cook_options			  = {};
				const string_t texture_asset_relative = get_texture_default_asset_relative(desc.source_base_name);
				const bool	   read_cook_options	  = editor_asset_writer_t::read_cook_options(texture_asset_relative.c_str(), cook_options);
				SFG_ASSERT(read_cook_options);
				if (!read_cook_options)
					continue;
				const string_t						 texture_source_relative = get_texture_default_source_relative(desc.source_base_name);
				const editor_asset_write_file_desc_t write_desc{
					.cook_options			  = &cook_options,
					.parent_path			  = default_assets_dir,
					.name					  = desc.asset_name,
					.source_name			  = desc.source_base_name,
					.source_extension		  = "png",
					.source_template_relative = texture_source_relative.c_str(),
					.guid					  = desc.guid,
					.asset_type				  = editor_asset_type_e::texture,
					.allow_overwrite		  = true,
				};
				editor_asset_t asset	  = {};
				string_t	   asset_path = {};
				bool		   created	  = editor_asset_writer_t::write_file_asset(write_desc, &asset, &asset_path);
				if (created)
					created = editor_asset_cooker_t::cook_texture(asset, desc.asset_name);

				SFG_ASSERT(created);
			}
		}

		void ensure_cubemap_assets(const char* default_assets_dir)
		{
			const default_file_asset_desc_t default_cubemap_assets[] = {
				{.asset_name = "default_sky_qwantani_dusk_2", .source_base_name = "default_sky_qwantani_dusk_2", .guid = DEFAULT_QWANTANI_DUSK_CUBEMAP_ASSET_GUID},
			};

			for (const default_file_asset_desc_t& desc : default_cubemap_assets)
			{
				if (is_default_asset_ready(default_assets_dir, desc.asset_name, desc.guid, editor_asset_type_e::cubemap, 0))
					continue;

				nlohmann::json cook_options			  = {};
				const string_t cubemap_asset_relative = get_cubemap_default_asset_relative(desc.source_base_name);
				const bool	   read_cook_options	  = editor_asset_writer_t::read_cook_options(cubemap_asset_relative.c_str(), cook_options);
				SFG_ASSERT(read_cook_options);
				if (!read_cook_options)
					continue;

				const string_t						 source_relative = get_cubemap_default_source_relative(desc.source_base_name);
				const editor_asset_write_file_desc_t write_desc{
					.cook_options			  = &cook_options,
					.parent_path			  = default_assets_dir,
					.name					  = desc.asset_name,
					.source_name			  = desc.source_base_name,
					.source_extension		  = "hdr",
					.source_template_relative = source_relative.c_str(),
					.guid					  = desc.guid,
					.asset_type				  = editor_asset_type_e::cubemap,
					.allow_overwrite		  = true,
				};
				editor_asset_t asset	  = {};
				string_t	   asset_path = {};
				bool		   created	  = editor_asset_writer_t::write_file_asset(write_desc, &asset, &asset_path);
				if (created)
					created = editor_asset_cooker_t::cook_cubemap(asset, desc.asset_name);

				SFG_ASSERT(created);
			}
		}

		void ensure_embedded_assets(const char* default_assets_dir)
		{
			const default_embedded_asset_desc_t default_embedded_assets[] = {
				{.asset_name		  = "default_material_gbuffer",
				 .asset_relative_path = EDITOR_DEFAULT_MATERIALS "default_material_gbuffer.sfg_asset",
				 .guid				  = DEFAULT_GBUFFER_MATERIAL_ASSET_GUID,
				 .asset_type		  = editor_asset_type_e::material,
				 .sub_type			  = static_cast<u8>(editor_material_type_e::gbuffer)},
				{.asset_name		  = "default_material_forward",
				 .asset_relative_path = EDITOR_DEFAULT_MATERIALS "default_material_forward.sfg_asset",
				 .guid				  = DEFAULT_FORWARD_MATERIAL_ASSET_GUID,
				 .asset_type		  = editor_asset_type_e::material,
				 .sub_type			  = static_cast<u8>(editor_material_type_e::forward)},
				{.asset_name		  = "default_material_unlit",
				 .asset_relative_path = EDITOR_DEFAULT_MATERIALS "default_material_unlit.sfg_asset",
				 .guid				  = DEFAULT_UNLIT_MATERIAL_ASSET_GUID,
				 .asset_type		  = editor_asset_type_e::material,
				 .sub_type			  = static_cast<u8>(editor_material_type_e::unlit)},
				{.asset_name		  = "default_material_skybox_cube",
				 .asset_relative_path = EDITOR_DEFAULT_MATERIALS "default_material_skybox_cube.sfg_asset",
				 .guid				  = DEFAULT_CUBE_SKYBOX_MATERIAL_ASSET_GUID,
				 .asset_type		  = editor_asset_type_e::material,
				 .sub_type			  = static_cast<u8>(editor_material_type_e::skybox)},
				{.asset_name		  = "default_material_skybox_gradient",
				 .asset_relative_path = EDITOR_DEFAULT_MATERIALS "default_material_skybox_gradient.sfg_asset",
				 .guid				  = DEFAULT_GRADIENT_SKYBOX_MATERIAL_ASSET_GUID,
				 .asset_type		  = editor_asset_type_e::material,
				 .sub_type			  = static_cast<u8>(editor_material_type_e::skybox)},
				{.asset_name = "default_physical_material", .asset_relative_path = EDITOR_DEFAULT_MATERIALS "default_physical_material.sfg_asset", .guid = DEFAULT_PHYSICAL_MATERIAL_ASSET_GUID, .asset_type = editor_asset_type_e::physical_material},
				{.asset_name		  = "default_sampler_linear",
				 .asset_relative_path = EDITOR_DEFAULT_SAMPLERS "default_sampler_linear.sfg_asset",
				 .guid				  = DEFAULT_LINEAR_SAMPLER_ASSET_GUID,
				 .asset_type		  = editor_asset_type_e::texture_sampler,
				 .sub_type			  = static_cast<u8>(editor_texture_sampler_type_e::linear)},
				{.asset_name		  = "default_sampler_nearest",
				 .asset_relative_path = EDITOR_DEFAULT_SAMPLERS "default_sampler_nearest.sfg_asset",
				 .guid				  = DEFAULT_NEAREST_SAMPLER_ASSET_GUID,
				 .asset_type		  = editor_asset_type_e::texture_sampler,
				 .sub_type			  = static_cast<u8>(editor_texture_sampler_type_e::nearest)},
				{.asset_name		  = "default_sampler_linear_repeat",
				 .asset_relative_path = EDITOR_DEFAULT_SAMPLERS "default_sampler_linear_repeat.sfg_asset",
				 .guid				  = DEFAULT_LINEAR_SAMPLER_REPEAT_ASSET_GUID,
				 .asset_type		  = editor_asset_type_e::texture_sampler,
				 .sub_type			  = static_cast<u8>(editor_texture_sampler_type_e::linear_repeat)},
				{.asset_name		  = "default_sampler_nearest_repeat",
				 .asset_relative_path = EDITOR_DEFAULT_SAMPLERS "default_sampler_nearest_repeat.sfg_asset",
				 .guid				  = DEFAULT_NEAREST_SAMPLER_REPEAT_ASSET_GUID,
				 .asset_type		  = editor_asset_type_e::texture_sampler,
				 .sub_type			  = static_cast<u8>(editor_texture_sampler_type_e::nearest_repeat)},
				{.asset_name		  = "default_sampler_anisotropic",
				 .asset_relative_path = EDITOR_DEFAULT_SAMPLERS "default_sampler_anisotropic.sfg_asset",
				 .guid				  = DEFAULT_ANISOTROPIC_SAMPLER_ASSET_GUID,
				 .asset_type		  = editor_asset_type_e::texture_sampler,
				 .sub_type			  = static_cast<u8>(editor_texture_sampler_type_e::anisotropic)},
				{.asset_name		  = "default_sampler_anisotropic_repeat",
				 .asset_relative_path = EDITOR_DEFAULT_SAMPLERS "default_sampler_anisotropic_repeat.sfg_asset",
				 .guid				  = DEFAULT_ANISOTROPIC_SAMPLER_REPEAT_ASSET_GUID,
				 .asset_type		  = editor_asset_type_e::texture_sampler,
				 .sub_type			  = static_cast<u8>(editor_texture_sampler_type_e::anisotropic_repeat)},
			};

			for (const default_embedded_asset_desc_t& desc : default_embedded_assets)
			{
				if (is_default_asset_ready(default_assets_dir, desc.asset_name, desc.guid, desc.asset_type, desc.sub_type))
					continue;

				nlohmann::json embedded_source		= {};
				const bool	   read_embedded_source = editor_asset_writer_t::read_embedded_source(desc.asset_relative_path, embedded_source);
				SFG_ASSERT(read_embedded_source);
				if (!read_embedded_source)
					continue;

				const editor_asset_write_embedded_desc_t write_desc{
					.embedded_source = &embedded_source,
					.parent_path	 = default_assets_dir,
					.name			 = desc.asset_name,
					.guid			 = desc.guid,
					.asset_type		 = desc.asset_type,
					.sub_type		 = desc.sub_type,
					.allow_overwrite = true,
				};
				editor_asset_t asset	  = {};
				string_t	   asset_path = {};
				bool		   created	  = editor_asset_writer_t::write_embedded_asset(write_desc, &asset, &asset_path);
				if (created && desc.asset_type == editor_asset_type_e::material)
					created = editor_asset_cooker_t::cook_material(asset, desc.asset_name);
				else if (created && desc.asset_type == editor_asset_type_e::physical_material)
					created = editor_asset_cooker_t::cook_physical_material(asset, desc.asset_name);
				else if (created && desc.asset_type == editor_asset_type_e::texture_sampler)
					created = editor_asset_cooker_t::cook_texture_sampler(asset, desc.asset_name);

				SFG_ASSERT(created);
			}
		}
	}

	void editor_default_asset_seeder_t::ensure(const char* default_assets_dir)
	{
		ensure_shader_assets(default_assets_dir);
		ensure_texture_assets(default_assets_dir);
		ensure_cubemap_assets(default_assets_dir);
		ensure_embedded_assets(default_assets_dir);
	}
}
