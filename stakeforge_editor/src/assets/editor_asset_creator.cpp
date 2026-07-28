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
#include "assets/editor_asset_path.hpp"
#include "assets/editor_asset_writer.hpp"
#include "editor_directories.hpp"

#include <sfg/data/string_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/resources/animation_graph_def.hpp>
#include <sfg/runtime/resources/curve_def.hpp>
#include <sfg/runtime/resources/shader_cook.hpp>
#include <sfg/runtime/resources/shader_types.hpp>
#include <sfg/serialization/serialization.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
#define COMMON_SHADERS			  "common/shaders/"
#define EDITOR_SHADERS			  "editor/resource_pack/shaders/"
#define EDITOR_TEMPLATE_MATERIALS "editor_templates/materials/"
#define EDITOR_TEMPLATE_SAMPLERS  "editor_templates/samplers/"
#define EDITOR_TEMPLATE_SCRIPTS	  "editor_templates/scripts/"

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
			case editor_material_type_e::transparent:
				return EDITOR_TEMPLATE_MATERIALS "material_transparent.sfg_asset";
			case editor_material_type_e::opaque_unlit:
				return EDITOR_TEMPLATE_MATERIALS "material_opaque_unlit.sfg_asset";
			case editor_material_type_e::transparent_unlit:
				return EDITOR_TEMPLATE_MATERIALS "material_transparent_unlit.sfg_asset";
			case editor_material_type_e::skybox:
				return EDITOR_TEMPLATE_MATERIALS "material_skybox.sfg_asset";
			case editor_material_type_e::sprite_lit:
				return EDITOR_TEMPLATE_MATERIALS "material_sprite_lit.sfg_asset";
			case editor_material_type_e::sprite_unlit:
				return EDITOR_TEMPLATE_MATERIALS "material_sprite_unlit.sfg_asset";
			case editor_material_type_e::particle:
				return EDITOR_TEMPLATE_MATERIALS "material_particle.sfg_asset";
			default:
				return EDITOR_TEMPLATE_MATERIALS "material_opaque.sfg_asset";
			}
		}

		const char* get_shader_template_relative(shader_type_e shader_type, editor_object_shader_template_e object_shader_template)
		{
			switch (shader_type)
			{
			case shader_type_e::object_shader:
				return object_shader_template == editor_object_shader_template_e::unlit ? COMMON_SHADERS "world/object_unlit.hlsl" : COMMON_SHADERS "world/object_lit.hlsl";
			case shader_type_e::post_process_shader:
				return COMMON_SHADERS "world/post_combiner.hlsl";
			case shader_type_e::ui_shader:
				return EDITOR_SHADERS "editor_ui_default.hlsl";
			case shader_type_e::ui_text_shader:
				return EDITOR_SHADERS "editor_ui_text_grayscale.hlsl";
			case shader_type_e::skybox_shader:
				return COMMON_SHADERS "world/skybox_cube.hlsl";
			case shader_type_e::sprite_lit_shader:
				return COMMON_SHADERS "world/sprite_lit.hlsl";
			case shader_type_e::sprite_unlit_shader:
				return COMMON_SHADERS "world/sprite_unlit.hlsl";
			case shader_type_e::particle_shader:
				return COMMON_SHADERS "world/particle.hlsl";
			default:
				return COMMON_SHADERS "world/object_lit.hlsl";
			}
		}

		const char* get_script_template_relative(editor_script_template_e script_template)
		{
			switch (script_template)
			{
			case editor_script_template_e::component:
				return EDITOR_TEMPLATE_SCRIPTS "component.cs";
			case editor_script_template_e::world_script:
				return EDITOR_TEMPLATE_SCRIPTS "world_script.cs";
			case editor_script_template_e::class_script:
				return EDITOR_TEMPLATE_SCRIPTS "class.cs";
			default:
				return nullptr;
			}
		}

		bool create_shader_asset(const editor_asset_create_desc_t& desc, const char* parent_path, editor_asset_t* out_asset, string_t* out_asset_path)
		{
			const shader_type_e shader_type	 = static_cast<shader_type_e>(desc.sub_type);
			nlohmann::json		cook_options = {};
			build_shader_template_cook_options(shader_type, cook_options);
			const editor_asset_write_file_desc_t write_desc{
				.cook_options			  = &cook_options,
				.parent_path			  = parent_path,
				.name					  = desc.name,
				.source_name			  = desc.source_name,
				.source_extension		  = "hlsl",
				.source_template_relative = get_shader_template_relative(shader_type, desc.object_shader_template),
				.guid					  = desc.guid,
				.asset_type				  = editor_asset_type_e::shader,
				.sub_type				  = desc.sub_type,
				.allow_overwrite		  = desc.allow_overwrite,
			};
			return editor_asset_writer_t::write_file_asset(write_desc, out_asset, out_asset_path);
		}

		bool create_material_asset(const editor_asset_create_desc_t& desc, const char* parent_path, editor_asset_t* out_asset, string_t* out_asset_path)
		{
			const editor_material_type_e material_type	 = static_cast<editor_material_type_e>(desc.sub_type);
			nlohmann::json				 embedded_source = {};
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
			const char*	   template_relative = EDITOR_TEMPLATE_SAMPLERS "sampler_linear_repeat.sfg_asset";
			nlohmann::json embedded_source	 = {};
			if (!editor_asset_writer_t::read_embedded_source(template_relative, embedded_source))
			{
				SFG_ERR("failed to read texture sampler asset template {0}", template_relative);
				return false;
			}

			const u8								 sub_type = static_cast<u8>(editor_texture_sampler_type_e::linear_repeat);
			const editor_asset_write_embedded_desc_t write_desc{
				.embedded_source = &embedded_source,
				.parent_path	 = parent_path,
				.name			 = desc.name,
				.guid			 = desc.guid,
				.asset_type		 = editor_asset_type_e::texture_sampler,
				.sub_type		 = sub_type,
				.allow_overwrite = desc.allow_overwrite,
			};
			return editor_asset_writer_t::write_embedded_asset(write_desc, out_asset, out_asset_path);
		}

		bool create_physical_material_asset(const editor_asset_create_desc_t& desc, const char* parent_path, editor_asset_t* out_asset, string_t* out_asset_path)
		{
			const char*	   template_relative = EDITOR_TEMPLATE_MATERIALS "physical_material.sfg_asset";
			nlohmann::json embedded_source	 = {};
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

		bool create_curve_asset(const editor_asset_create_desc_t& desc, const char* parent_path, editor_asset_t* out_asset, string_t* out_asset_path)
		{
			curve_def_t	   definition	   = {};
			nlohmann::json embedded_source = nlohmann::json::object();

			if (!reflection_registry_t::get().type_to_json(type_id_t<curve_def_t>::value, &definition, nullptr, embedded_source))
			{
				SFG_ERR("failed to serialize initial curve definition");
				return false;
			}

			embedded_source["schema"] = "sfg.schema.curve";

			const editor_asset_write_embedded_desc_t write_desc{
				.embedded_source = &embedded_source,
				.parent_path	 = parent_path,
				.name			 = desc.name,
				.guid			 = desc.guid,
				.asset_type		 = editor_asset_type_e::curve,
				.sub_type		 = desc.sub_type,
				.allow_overwrite = desc.allow_overwrite,
			};

			return editor_asset_writer_t::write_embedded_asset(write_desc, out_asset, out_asset_path);
		}

		bool create_animation_graph_asset(const editor_asset_create_desc_t& desc, const char* parent_path, editor_asset_t* out_asset, string_t* out_asset_path)
		{
			animation_graph_def_t definition = {};
			definition.name					 = desc.name;
			definition.nodes.push_back({});
			definition.entry_node_id  = 1;
			definition.output_node_id = 1;
			definition.next_id		  = 3;

			animation_graph_node_def_t& node = definition.nodes.back();
			node.type						 = animation_graph_node_type_e::asm_node;
			node.id							 = 1;
			node.asm_node.states.push_back({});
			node.asm_node.first_state_id = 2;
			node.name					 = "ASM Node";

			animation_graph_asm_state_def_t& state = node.asm_node.states.back();
			state.id							   = 2;
			state.name							   = "State";

			nlohmann::json embedded_source = nlohmann::json::object();

			if (!reflection_registry_t::get().type_to_json(type_id_t<animation_graph_def_t>::value, &definition, nullptr, embedded_source))
			{
				SFG_ERR("failed to serialize initial animation graph definition");
				return false;
			}

			embedded_source["schema"] = "sfg.schema.animation_graph";

			const editor_asset_write_embedded_desc_t write_desc{
				.embedded_source = &embedded_source,
				.parent_path	 = parent_path,
				.name			 = desc.name,
				.guid			 = desc.guid,
				.asset_type		 = editor_asset_type_e::animation_graph,
				.sub_type		 = desc.sub_type,
				.allow_overwrite = desc.allow_overwrite,
			};

			return editor_asset_writer_t::write_embedded_asset(write_desc, out_asset, out_asset_path);
		}

		bool create_prefab_asset(const editor_asset_create_desc_t& desc, const char* parent_path, editor_asset_t* out_asset, string_t* out_asset_path)
		{
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
		editor_asset_t asset	  = {};
		string_t	   asset_path = {};
		bool		   result	  = false;

		const editor_asset_tree_t& tree		   = editor_asset_manager_t::get().get_asset_tree();
		const editor_asset_node_t& parent_node = tree.value(desc.parent_node);
		const char* const		   parent_path = parent_node.full_path.c_str();

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
		case editor_asset_type_e::curve:
			result = create_curve_asset(desc, parent_path, &asset, &asset_path);
			if (result)
				result = editor_asset_cooker_t::cook_curve(asset, desc.name);
			break;
		case editor_asset_type_e::animation_graph:
			result = create_animation_graph_asset(desc, parent_path, &asset, &asset_path);
			if (result)
				result = editor_asset_cooker_t::cook_animation_graph(asset, desc.name);
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

	bool editor_asset_creator_t::create_script(const editor_script_create_desc_t& desc, string_t* out_path)
	{
		if (!editor_directories_t::is_valid_csharp_identifier(desc.name))
			return false;

		const char* template_relative = get_script_template_relative(desc.script_template);

		if (template_relative == nullptr)
			return false;

		const editor_asset_tree_t& tree		   = editor_asset_manager_t::get().get_asset_tree();
		const editor_asset_node_t& parent_node = tree.value(desc.parent_node);
		SFG_ASSERT(parent_node.type == editor_asset_node_type_e::folder);

		string_t script_path = editor_asset_path_t::normalize_directory(parent_node.full_path.c_str());
		script_path += desc.name;
		script_path += ".cs";

		if (!desc.allow_overwrite && file_system_t::exists(script_path.c_str()))
			return false;

		string_t template_path = file_system_t::get_running_directory();
		template_path += template_relative;

		if (!file_system_t::exists(template_path.c_str()))
		{
			SFG_ERR("C# script template does not exist: {0}", template_path);
			return false;
		}

		string_t contents = file_system_t::read_file_as_string(template_path.c_str());
		string_util::replace_all(contents, "#SCRIPT_NAME#", desc.name);

		if (!serializer_t::write_to_file(contents, script_path.c_str()))
			return false;

		if (out_path != nullptr)
			*out_path = std::move(script_path);

		return true;
	}
}
