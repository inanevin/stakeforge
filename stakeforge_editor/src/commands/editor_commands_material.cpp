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

#include "commands/editor_commands_material.hpp"
#include "assets/editor_asset.hpp"
#include "assets/editor_asset_cooker.hpp"
#include "assets/editor_asset_io.hpp"
#include "assets/editor_asset_manager.hpp"
#include "editor_command_system.hpp"

#include <sfg/io/log.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

#include <algorithm>

namespace sfg
{
	namespace
	{
		chunk_handle32_t copy_material_ids_to_aux(editor_command_system_t& system, span_t<const sid_t> materials)
		{
			sid_t*				   dst	  = nullptr;
			const chunk_handle32_t handle = system.get_aux_data().allocate<sid_t>(materials.size, dst);
			SFG_MEMCPY(dst, materials.data, sizeof(sid_t) * materials.size);
			return handle;
		}

		chunk_handle32_t copy_json_text_to_aux(editor_command_system_t& system, const nlohmann::json& json)
		{
			const string_t		   text	  = json.dump();
			const chunk_handle32_t handle = system.get_aux_data().allocate_bytes(text.size(), alignof(char));
			SFG_MEMCPY(system.get_aux_data().get<char>(handle), text.data(), text.size());
			return handle;
		}

		chunk_handle32_t copy_material_jsons_to_aux(editor_command_system_t& system, span_t<const material_def_t> materials)
		{
			const chunk_handle32_t handle = system.get_aux_data().allocate_bytes(sizeof(chunk_handle32_t) * materials.size, alignof(chunk_handle32_t));
			chunk_handle32_t*	   dst	  = system.get_aux_data().get<chunk_handle32_t>(handle);
			for (size_t i = 0; i < materials.size; ++i)
				dst[i] = copy_json_text_to_aux(system, nlohmann::json(materials.data[i]));
			return handle;
		}

		void free_jsons(editor_command_system_t& system, chunk_handle32_t jsons_handle, u32 count)
		{
			if (!jsons_handle)
				return;

			chunk_handle32_t* jsons = system.get_aux_data().get<chunk_handle32_t>(jsons_handle);
			for (u32 i = 0; i < count; ++i)
			{
				if (jsons[i])
				{
					system.get_aux_data().free(jsons[i]);
					jsons[i] = {};
				}
			}
		}

		void free_material_edit_payload(editor_command_system_t& system, editor_command_material_edit_payload_t& payload)
		{
			free_jsons(system, payload.previous_jsons, payload.count);
			free_jsons(system, payload.post_jsons, payload.count);
			if (payload.material_ids)
			{
				system.get_aux_data().free(payload.material_ids);
				payload.material_ids = {};
			}
			if (payload.previous_jsons)
			{
				system.get_aux_data().free(payload.previous_jsons);
				payload.previous_jsons = {};
			}
			if (payload.post_jsons)
			{
				system.get_aux_data().free(payload.post_jsons);
				payload.post_jsons = {};
			}
		}

		bool material_defs_equal(span_t<const material_def_t> a, span_t<const material_def_t> b)
		{
			if (a.size != b.size)
				return false;

			for (size_t i = 0; i < a.size; ++i)
			{
				if (nlohmann::json(a.data[i]) != nlohmann::json(b.data[i]))
					return false;
			}
			return true;
		}

		bool load_shader_definition(resource_handle_t shader, shader_data_definition_t& out_definition)
		{
			out_definition = {};
			if (shader == NULL_RESOURCE_HANDLE)
				return true;

			const editor_asset_t* asset = editor_asset_manager_t::get().find_asset(shader);
			if (asset == nullptr || asset->asset_type != editor_asset_type_e::shader)
				return false;

			const nlohmann::json embedded_source = editor_asset_io_t::get_embedded_source_json(*asset);
			if (!embedded_source.is_object())
				return true;

			embedded_source.get_to(out_definition);
			return true;
		}

		void preserve_matching_values(material_def_t& dst, const material_def_t& src)
		{
			for (material_texture_value_t& texture : dst.textures)
			{
				const auto it = std::find_if(src.textures.begin(), src.textures.end(), [&](const material_texture_value_t& value) { return value.name == texture.name; });
				if (it != src.textures.end())
					texture.texture = it->texture;
			}

			for (material_sampler_value_t& sampler : dst.samplers)
			{
				const auto it = std::find_if(src.samplers.begin(), src.samplers.end(), [&](const material_sampler_value_t& value) { return value.name == sampler.name; });
				if (it != src.samplers.end())
					sampler.sampler = it->sampler;
			}

			for (material_param_value_t& parameter : dst.parameters)
			{
				const auto it = std::find_if(src.parameters.begin(), src.parameters.end(), [&](const material_param_value_t& value) { return value.name == parameter.name && value.type == parameter.type; });
				if (it != src.parameters.end())
				{
					parameter.hint = it->hint;
					for (u8 i = 0; i < 4; ++i)
					{
						if (parameter.type == shader_param_type_e::u32)
							parameter.value_u32[i] = it->value_u32[i];
						else
							parameter.value[i] = it->value[i];
					}
				}
			}
		}

		bool refresh_material_from_shader(const material_def_t& src, resource_handle_t shader, material_def_t& out_material)
		{
			shader_data_definition_t definition = {};
			if (!load_shader_definition(shader, definition))
				return false;

			out_material				  = material_def_from_shader_def(definition, shader);
			out_material.pass_flags		  = src.pass_flags;
			out_material.double_sided	  = src.double_sided;
			out_material.use_alpha_cutoff = src.use_alpha_cutoff;
			preserve_matching_values(out_material, src);
			return true;
		}

		bool build_shader_post_materials(span_t<const material_def_t> previous, span_t<const resource_handle_t> post_shaders, vector_t<material_def_t>& out_post)
		{
			out_post.resize(0);
			out_post.reserve(previous.size);
			for (size_t i = 0; i < previous.size; ++i)
			{
				material_def_t material = {};
				if (!refresh_material_from_shader(previous.data[i], post_shaders.data[i], material))
					return false;
				out_post.push_back(material);
			}
			return true;
		}

		bool apply_material_jsons(editor_command_system_t& system, const editor_command_material_edit_payload_t& payload, chunk_handle32_t jsons_handle)
		{
			editor_asset_manager_t& asset_manager = editor_asset_manager_t::get();
			const sid_t*			material_ids  = system.get_aux_data().get<sid_t>(payload.material_ids);
			const chunk_handle32_t* jsons		  = system.get_aux_data().get<chunk_handle32_t>(jsons_handle);

			for (u32 i = 0; i < payload.count; ++i)
			{
				const char*			 json_text = system.get_aux_data().get<char>(jsons[i]);
				const nlohmann::json embedded  = nlohmann::json::parse(json_text, json_text + jsons[i].size, nullptr, false);
				if (embedded.is_discarded())
				{
					SFG_ERR("failed to parse material edit json for asset {0}", material_ids[i]);
					return false;
				}

				const editor_asset_node_handle_t node = asset_manager.find_asset_node_handle(material_ids[i]);
				if (node.is_null())
				{
					SFG_ERR("failed to find material asset node {0}", material_ids[i]);
					return false;
				}

				const editor_asset_tree_t& tree		  = asset_manager.get_asset_tree();
				const editor_asset_node_t& asset_node = tree.value(node);
				editor_asset_t			   asset	  = {};
				if (!editor_asset_io_t::read_asset(asset_node.full_path.c_str(), asset))
					return false;

				editor_asset_io_t::set_embedded_source_json(asset, embedded);
				if (!editor_asset_io_t::write_asset(asset_node.full_path.c_str(), asset))
					return false;
				if (!editor_asset_cooker_t::cook_material(asset))
					return false;
				if (!asset_manager.reload_asset_node(node))
					return false;
			}
			return true;
		}

		bool material_edit_undo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_material_edit_payload_t& payload = system.get_payload_as<editor_command_material_edit_payload_t>(command);
			return apply_material_jsons(system, payload, payload.previous_jsons);
		}

		bool material_edit_redo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_material_edit_payload_t& payload = system.get_payload_as<editor_command_material_edit_payload_t>(command);
			return apply_material_jsons(system, payload, payload.post_jsons);
		}

		bool material_edit_cleanup(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_material_edit_payload_t& payload = system.get_payload_as<editor_command_material_edit_payload_t>(command);
			free_material_edit_payload(system, payload);
			return true;
		}

		bool issue_material_command(span_t<const sid_t> materials, span_t<const material_def_t> previous, span_t<const material_def_t> post, const char* debug_name, editor_command_type_e type)
		{
			if (materials.size == 0 || previous.size != materials.size || post.size != materials.size)
				return false;
			if (material_defs_equal(previous, post))
				return true;

			editor_command_system_t& command_system = editor_command_system_t::get();

			editor_command_material_edit_payload_t payload = {};
			payload.material_ids						   = copy_material_ids_to_aux(command_system, materials);
			payload.previous_jsons						   = copy_material_jsons_to_aux(command_system, previous);
			payload.post_jsons							   = copy_material_jsons_to_aux(command_system, post);
			payload.count								   = static_cast<u32>(materials.size);

			const editor_command_issue_desc_t desc{
				.undo		= material_edit_undo,
				.redo		= material_edit_redo,
				.cleanup	= material_edit_cleanup,
				.debug_name = debug_name,
				.type		= type,
				.notify		= false,
			};

			const editor_command_handle_t handle = command_system.issue_command(desc, payload);
			if (handle.is_null())
			{
				SFG_ERR("failed to issue material edit command");
				return false;
			}

			return true;
		}
	}

	bool editor_command_material_edit_t::edit(span_t<const sid_t> materials, span_t<const material_def_t> previous, span_t<const material_def_t> post)
	{
		return issue_material_command(materials, previous, post, "Material Edit", editor_command_type_e::material_edit);
	}

	bool editor_command_shader_edit_t::edit(span_t<const sid_t> materials, span_t<const material_def_t> previous, span_t<const resource_handle_t> post_shaders)
	{
		if (materials.size == 0 || previous.size != materials.size || post_shaders.size != materials.size)
			return false;

		vector_t<material_def_t> post;
		if (!build_shader_post_materials(previous, post_shaders, post))
			return false;
		return issue_material_command(materials, previous, {.data = post.data(), .size = post.size()}, "Shader Edit", editor_command_type_e::shader_edit);
	}
}
