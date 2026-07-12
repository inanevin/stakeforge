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

#include "assets/editor_asset_creator.hpp"
#include "assets/editor_asset.hpp"
#include "assets/editor_asset_cooker.hpp"
#include "assets/editor_asset_io.hpp"
#include "assets/editor_asset_manager.hpp"
#include "assets/editor_asset_writer.hpp"

#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/runtime/resources/shader_cook.hpp>
#include <sfg/runtime/resources/shader_types.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
#define COMMON_SHADERS			  "common/shaders/"
#define EDITOR_TEMPLATE_MATERIALS "editor_templates/materials/"
#define EDITOR_TEMPLATE_SAMPLERS  "editor_templates/samplers/"

	namespace
	{
		void build_shader_template_cook_options(shader_type_e shader_type, nlohmann::json& out)
		{
			out["schema"]		= "sfg.schema.shader";
			out["include_dirs"] = {COMMON_SHADERS, COMMON_SHADERS "world/"};
			out["type"]			= static_cast<u8>(shader_type);
		}

		const char* get_material_template_relative(editor_material_type_e material_type)
		{
			switch (material_type)
			{
			case editor_material_type_e::forward:
				return EDITOR_TEMPLATE_MATERIALS "material_forward.sfg_asset";
			default:
				return EDITOR_TEMPLATE_MATERIALS "material_gbuffer.sfg_asset";
			}
		}

		const char* get_shader_template_relative(shader_type_e shader_type)
		{
			switch (shader_type)
			{
			case shader_type_e::opaque_shader:
				return COMMON_SHADERS "world/gbuffer_lit.hlsl";
			case shader_type_e::transparent_shader:
				return COMMON_SHADERS "world/forward.hlsl";
			case shader_type_e::post_process_shader:
				return COMMON_SHADERS "world/forward.hlsl";
			case shader_type_e::ui_shader:
				return COMMON_SHADERS "world/forward.hlsl";
			case shader_type_e::ui_text_shader:
				return COMMON_SHADERS "world/forward.hlsl";
			default:
				return COMMON_SHADERS "world/gbuffer_lit.hlsl";
			}
		}

		const char* get_texture_sampler_template_relative(editor_texture_sampler_type_e sampler_type)
		{
			switch (sampler_type)
			{
			case editor_texture_sampler_type_e::nearest:
				return EDITOR_TEMPLATE_SAMPLERS "sampler_nearest.sfg_asset";
			case editor_texture_sampler_type_e::linear_repeat:
				return EDITOR_TEMPLATE_SAMPLERS "sampler_linear_repeat.sfg_asset";
			case editor_texture_sampler_type_e::nearest_repeat:
				return EDITOR_TEMPLATE_SAMPLERS "sampler_nearest_repeat.sfg_asset";
			case editor_texture_sampler_type_e::anisotropic:
				return EDITOR_TEMPLATE_SAMPLERS "sampler_anisotropic.sfg_asset";
			default:
				return EDITOR_TEMPLATE_SAMPLERS "sampler_linear.sfg_asset";
			}
		}

		bool create_shader_asset(const editor_asset_create_desc_t& desc, const char* parent_path, editor_asset_t* out_asset, string_t* out_asset_path)
		{
			const shader_type_e shader_type = static_cast<shader_type_e>(desc.sub_type);
			nlohmann::json		cook_options;
			build_shader_template_cook_options(shader_type, cook_options);
			const editor_asset_write_file_desc_t write_desc{
				.cook_options			  = &cook_options,
				.parent_path			  = parent_path,
				.name					  = desc.name,
				.source_name			  = desc.source_name,
				.source_extension		  = "hlsl",
				.source_template_relative = get_shader_template_relative(shader_type),
				.guid					  = desc.guid,
				.asset_type				  = editor_asset_type_e::shader,
				.sub_type				  = desc.sub_type,
				.allow_overwrite		  = desc.allow_overwrite,
			};
			return editor_asset_writer_t::write_file_asset(write_desc, out_asset, out_asset_path);
		}

		bool create_material_asset(const editor_asset_create_desc_t& desc, const char* parent_path, editor_asset_t* out_asset, string_t* out_asset_path)
		{
			const editor_material_type_e material_type = static_cast<editor_material_type_e>(desc.sub_type);
			nlohmann::json				 embedded_source;
			if (!editor_asset_writer_t::read_embedded_source(get_material_template_relative(material_type), embedded_source))
			{
				SFG_ERR("failed to read material asset template {0}", get_material_template_relative(material_type));
				return false;
			}

			const editor_asset_write_embedded_desc_t write_desc{
				.embedded_source = &embedded_source,
				.parent_path	 = parent_path,
				.name			 = desc.name,
				.guid			 = desc.guid,
				.asset_type		 = editor_asset_type_e::material,
				.sub_type		 = desc.sub_type,
				.allow_overwrite = desc.allow_overwrite,
			};
			return editor_asset_writer_t::write_embedded_asset(write_desc, out_asset, out_asset_path);
		}

		bool create_texture_sampler_asset(const editor_asset_create_desc_t& desc, const char* parent_path, editor_asset_t* out_asset, string_t* out_asset_path)
		{
			const editor_texture_sampler_type_e sampler_type = static_cast<editor_texture_sampler_type_e>(desc.sub_type);
			nlohmann::json						embedded_source;
			if (!editor_asset_writer_t::read_embedded_source(get_texture_sampler_template_relative(sampler_type), embedded_source))
			{
				SFG_ERR("failed to read texture sampler asset template {0}", get_texture_sampler_template_relative(sampler_type));
				return false;
			}

			const editor_asset_write_embedded_desc_t write_desc{
				.embedded_source = &embedded_source,
				.parent_path	 = parent_path,
				.name			 = desc.name,
				.guid			 = desc.guid,
				.asset_type		 = editor_asset_type_e::texture_sampler,
				.sub_type		 = desc.sub_type,
				.allow_overwrite = desc.allow_overwrite,
			};
			return editor_asset_writer_t::write_embedded_asset(write_desc, out_asset, out_asset_path);
		}

		bool create_physical_material_asset(const editor_asset_create_desc_t& desc, const char* parent_path, editor_asset_t* out_asset, string_t* out_asset_path)
		{
			const char*	   template_relative = EDITOR_TEMPLATE_MATERIALS "physical_material.sfg_asset";
			nlohmann::json embedded_source;
			if (!editor_asset_writer_t::read_embedded_source(template_relative, embedded_source))
			{
				SFG_ERR("failed to read physical material asset template {0}", template_relative);
				return false;
			}

			const editor_asset_write_embedded_desc_t write_desc{
				.embedded_source = &embedded_source,
				.parent_path	 = parent_path,
				.name			 = desc.name,
				.guid			 = desc.guid,
				.asset_type		 = editor_asset_type_e::physical_material,
				.sub_type		 = desc.sub_type,
				.allow_overwrite = desc.allow_overwrite,
			};
			return editor_asset_writer_t::write_embedded_asset(write_desc, out_asset, out_asset_path);
		}

		bool create_animation_state_machine_asset(const editor_asset_create_desc_t& desc, const char* parent_path, editor_asset_t* out_asset, string_t* out_asset_path)
		{
			const editor_asset_write_none_desc_t write_desc{
				.parent_path	 = parent_path,
				.name			 = desc.name,
				.guid			 = desc.guid,
				.asset_type		 = editor_asset_type_e::animation_state_machine,
				.sub_type		 = desc.sub_type,
				.allow_overwrite = desc.allow_overwrite,
			};
			return editor_asset_writer_t::write_none_source_asset(write_desc, out_asset, out_asset_path);
		}

		bool create_prefab_asset(const editor_asset_create_desc_t& desc, const char* parent_path, editor_asset_t* out_asset, string_t* out_asset_path)
		{
			SFG_ASSERT(desc.embedded_data != nullptr);

			const nlohmann::json embedded_source = nlohmann::json::parse(desc.embedded_data, nullptr, false);
			if (embedded_source.is_discarded())
			{
				SFG_ERR("failed to parse prefab embedded data");
				return false;
			}

			const editor_asset_write_embedded_desc_t write_desc{
				.embedded_source = &embedded_source,
				.parent_path	 = parent_path,
				.name			 = desc.name,
				.guid			 = desc.guid,
				.asset_type		 = editor_asset_type_e::prefab,
				.sub_type		 = desc.sub_type,
				.allow_overwrite = desc.allow_overwrite,
			};
			return editor_asset_writer_t::write_embedded_asset(write_desc, out_asset, out_asset_path);
		}

		bool create_world_asset(const editor_asset_create_desc_t& desc, const char* parent_path, editor_asset_t* out_asset, string_t* out_asset_path)
		{
			const editor_asset_write_none_desc_t write_desc{
				.parent_path	 = parent_path,
				.name			 = desc.name,
				.guid			 = desc.guid,
				.asset_type		 = editor_asset_type_e::world,
				.sub_type		 = desc.sub_type,
				.allow_overwrite = desc.allow_overwrite,
			};
			return editor_asset_writer_t::write_none_source_asset(write_desc, out_asset, out_asset_path);
		}
	}

	bool editor_asset_creator_t::create_asset(const editor_asset_create_desc_t& desc, editor_asset_t* out_asset)
	{
		editor_asset_t asset = {};
		string_t	   asset_path;
		bool		   result = false;

		const editor_asset_tree_t& tree = editor_asset_manager_t::get().get_asset_tree();
		SFG_ASSERT(!desc.parent_node.is_null());
		SFG_ASSERT(tree.is_valid(desc.parent_node));
		const editor_asset_node_t& parent_node = tree.value(desc.parent_node);
		SFG_ASSERT(parent_node.type == editor_asset_node_type_e::folder);
		SFG_ASSERT(!parent_node.full_path.empty());
		const char* const parent_path = parent_node.full_path.c_str();

		switch (desc.asset_type)
		{
		case editor_asset_type_e::shader:
			result = create_shader_asset(desc, parent_path, &asset, &asset_path);
			if (result)
			{
				shader_data_definition_t definition = {};
				result								= editor_asset_cooker_t::cook_shader(asset, desc.name, &definition);
				if (result)
				{
					editor_asset_io_t::set_embedded_source_json(asset, definition);
					result = editor_asset_io_t::write_asset(asset_path.c_str(), asset);
				}
			}
			break;
		case editor_asset_type_e::material:
			result = create_material_asset(desc, parent_path, &asset, &asset_path);
			if (result)
				result = editor_asset_cooker_t::cook_material(asset, desc.name);
			break;
		case editor_asset_type_e::texture_sampler:
			result = create_texture_sampler_asset(desc, parent_path, &asset, &asset_path);
			if (result)
				result = editor_asset_cooker_t::cook_texture_sampler(asset, desc.name);
			break;
		case editor_asset_type_e::physical_material:
			result = create_physical_material_asset(desc, parent_path, &asset, &asset_path);
			if (result)
				result = editor_asset_cooker_t::cook_physical_material(asset, desc.name);
			break;
		case editor_asset_type_e::animation_state_machine:
			result = create_animation_state_machine_asset(desc, parent_path, &asset, &asset_path);
			if (result)
				result = editor_asset_cooker_t::cook_animation_state_machine(asset, desc.name);
			break;
		case editor_asset_type_e::prefab:
			result = create_prefab_asset(desc, parent_path, &asset, &asset_path);
			if (result)
				result = editor_asset_cooker_t::cook_prefab(asset, desc.name);
			break;
		case editor_asset_type_e::world:
			result = create_world_asset(desc, parent_path, &asset, &asset_path);
			break;
		default:
			SFG_ASSERT(false);
			return false;
		}

		if (result && out_asset != nullptr)
			*out_asset = asset;
		return result;
	}
}
