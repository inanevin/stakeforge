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

#include "editor/assets/editor_asset_creator.hpp"
#include "editor/assets/editor_asset.hpp"
#include "editor/assets/editor_asset_cooker.hpp"
#include "editor/assets/editor_asset_io.hpp"
#include "editor/assets/editor_asset_manager.hpp"
#include "editor/assets/editor_asset_path.hpp"
#include "editor/assets/editor_asset_writer.hpp"
#include "editor/editor_directories.hpp"

#include <sfg/core/common/hashing.hpp>
#include <sfg/core/data/string_util.hpp>
#include <sfg/core/io/assert.hpp>
#include <sfg/core/io/file_system.hpp>
#include <sfg/core/io/log.hpp>
#include <sfg/core/reflection/reflection_registry.hpp>
#include <sfg/core/runtime/resources/animation_library_def.hpp>
#include <sfg/core/runtime/resources/curve_def.hpp>
#include <sfg/core/runtime/resources/ragdoll_def.hpp>
#include <sfg/core/runtime/resources/shader_cook.hpp>
#include <sfg/core/runtime/resources/shader_types.hpp>
#include <sfg/core/serialization/serialization.hpp>
#include <sfg/core/vendor/nhlohmann/json.hpp>

namespace sfg
{
#define COMMON_SHADERS			  "shaders/"
#define EDITOR_SHADERS			  "shaders/editor/"
#define EDITOR_TEMPLATE_MATERIALS "editor/templates/materials/"
#define EDITOR_TEMPLATE_SAMPLERS  "editor/templates/samplers/"

	namespace
	{
		void build_shader_template_cook_options(shader_type_e shader_type, nlohmann::json& out)
		{
			out["schema"]		= "sfg.schema.shader";
			out["include_dirs"] = {"assets/shaders/"};
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
				return object_shader_template == editor_object_shader_template_e::unlit ? COMMON_SHADERS "object/object_unlit.hlsl" : COMMON_SHADERS "object/object_lit.hlsl";
			case shader_type_e::canvas_ui:
				return COMMON_SHADERS "canvas/canvas_ui.hlsl";
			case shader_type_e::canvas_ui_text:
				return COMMON_SHADERS "canvas/canvas_ui_text.hlsl";
			case shader_type_e::canvas_ui_texture:
				return COMMON_SHADERS "canvas/canvas_ui_texture.hlsl";
			case shader_type_e::skybox_shader:
				return COMMON_SHADERS "sky/skybox_cube.hlsl";
			case shader_type_e::sprite_lit_shader:
				return COMMON_SHADERS "sprite/sprite_lit.hlsl";
			case shader_type_e::sprite_unlit_shader:
				return COMMON_SHADERS "sprite/sprite_unlit.hlsl";
			case shader_type_e::particle_shader:
				return COMMON_SHADERS "sprite/particle.hlsl";
			default:
				return COMMON_SHADERS "object/object_lit.hlsl";
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
			const char*	   template_relative = "editor/templates/physical_materials/physical_material.sfg_asset";
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

		bool create_animation_library_asset(const editor_asset_create_desc_t& desc, const char* parent_path, editor_asset_t* out_asset, string_t* out_asset_path)
		{
			animation_library_def_t definition		= {};
			nlohmann::json			embedded_source = nlohmann::json::object();

			if (!reflection_registry_t::get().type_to_json(type_id_t<animation_library_def_t>::value, &definition, nullptr, embedded_source))
			{
				SFG_ERR("failed to serialize initial animation library definition");
				return false;
			}

			embedded_source["schema"] = "sfg.schema.animation_library";

			const editor_asset_write_embedded_desc_t write_desc{
				.embedded_source = &embedded_source,
				.parent_path	 = parent_path,
				.name			 = desc.name,
				.guid			 = desc.guid,
				.asset_type		 = editor_asset_type_e::animation_library,
				.sub_type		 = desc.sub_type,
				.allow_overwrite = desc.allow_overwrite,
			};

			return editor_asset_writer_t::write_embedded_asset(write_desc, out_asset, out_asset_path);
		}

		bool create_ragdoll_asset(const editor_asset_create_desc_t& desc, const char* parent_path, editor_asset_t* out_asset, string_t* out_asset_path)
		{
			ragdoll_def_t  definition	   = {};
			nlohmann::json embedded_source = nlohmann::json::object();

			if (!reflection_registry_t::get().type_to_json(type_id_t<ragdoll_def_t>::value, &definition, nullptr, embedded_source))
			{
				SFG_ERR("failed to serialize initial ragdoll definition");
				return false;
			}

			embedded_source["schema"] = "sfg.schema.ragdoll";

			const editor_asset_write_embedded_desc_t write_desc{
				.embedded_source = &embedded_source,
				.parent_path	 = parent_path,
				.name			 = desc.name,
				.guid			 = desc.guid,
				.asset_type		 = editor_asset_type_e::ragdoll,
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
			const char*	   template_relative = "editor/templates/worlds/world.sfg_asset";
			nlohmann::json embedded_source	 = {};

			if (!editor_asset_writer_t::read_embedded_source(template_relative, embedded_source))
			{
				SFG_ERR("failed to read world asset template {0}", template_relative);
				return false;
			}

			const editor_asset_write_embedded_desc_t write_desc{
				.embedded_source = &embedded_source,
				.parent_path	 = parent_path,
				.name			 = desc.name,
				.guid			 = desc.guid,
				.asset_type		 = editor_asset_type_e::world,
				.sub_type		 = desc.sub_type,
				.allow_overwrite = desc.allow_overwrite,
			};

			return editor_asset_writer_t::write_embedded_asset(write_desc, out_asset, out_asset_path);
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

				result = editor_asset_cooker_t::cook_shader(asset, desc.name, &definition);

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
		case editor_asset_type_e::animation_library:
			result = create_animation_library_asset(desc, parent_path, &asset, &asset_path);

			if (result)
				result = editor_asset_cooker_t::cook_animation_library(asset, desc.name);

			break;
		case editor_asset_type_e::ragdoll:
			result = create_ragdoll_asset(desc, parent_path, &asset, &asset_path);

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
