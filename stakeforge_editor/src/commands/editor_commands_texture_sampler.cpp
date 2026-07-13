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

#include "commands/editor_commands_texture_sampler.hpp"
#include "assets/editor_asset.hpp"
#include "assets/editor_asset_cooker.hpp"
#include "assets/editor_asset_io.hpp"
#include "assets/editor_asset_manager.hpp"
#include "editor_command_system.hpp"

#include <sfg/common/type_id.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	namespace
	{
		nlohmann::json sampler_desc_to_json(const sampler_desc_t& sampler)
		{
			nlohmann::json json = nlohmann::json::object();
			if (!reflection_registry_t::get().type_to_json(type_id_t<sampler_desc_t>::value, const_cast<sampler_desc_t*>(&sampler), nullptr, json))
				return nlohmann::json::object();

			json["schema"] = "sfg.schema.texture_sampler";
			return json;
		}

		chunk_handle32_t copy_sampler_ids_to_aux(editor_command_system_t& system, span_t<const sid_t> samplers)
		{
			sid_t*				   dst	  = nullptr;
			const chunk_handle32_t handle = system.get_aux_data().allocate<sid_t>(samplers.size, dst);
			SFG_MEMCPY(dst, samplers.data, sizeof(sid_t) * samplers.size);
			return handle;
		}

		chunk_handle32_t copy_json_text_to_aux(editor_command_system_t& system, const nlohmann::json& json)
		{
			const string_t		   text	  = json.dump();
			const chunk_handle32_t handle = system.get_aux_data().allocate_bytes(text.size(), alignof(char));
			SFG_MEMCPY(system.get_aux_data().get<char>(handle), text.data(), text.size());
			return handle;
		}

		chunk_handle32_t copy_sampler_jsons_to_aux(editor_command_system_t& system, span_t<const sampler_desc_t> samplers)
		{
			const chunk_handle32_t handle = system.get_aux_data().allocate_bytes(sizeof(chunk_handle32_t) * samplers.size, alignof(chunk_handle32_t));
			chunk_handle32_t*	   dst	  = system.get_aux_data().get<chunk_handle32_t>(handle);
			for (size_t i = 0; i < samplers.size; ++i)
				dst[i] = copy_json_text_to_aux(system, sampler_desc_to_json(samplers.data[i]));
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

		void free_texture_sampler_edit_payload(editor_command_system_t& system, editor_command_texture_sampler_edit_payload_t& payload)
		{
			free_jsons(system, payload.previous_jsons, payload.count);
			free_jsons(system, payload.post_jsons, payload.count);
			if (payload.sampler_ids)
			{
				system.get_aux_data().free(payload.sampler_ids);
				payload.sampler_ids = {};
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

		bool sampler_descs_equal(span_t<const sampler_desc_t> a, span_t<const sampler_desc_t> b)
		{
			if (a.size != b.size)
				return false;

			for (size_t i = 0; i < a.size; ++i)
			{
				if (!(a.data[i] == b.data[i]))
					return false;
			}
			return true;
		}

		bool apply_texture_sampler_jsons(editor_command_system_t& system, const editor_command_texture_sampler_edit_payload_t& payload, chunk_handle32_t jsons_handle)
		{
			editor_asset_manager_t& asset_manager = editor_asset_manager_t::get();
			const sid_t*			sampler_ids	  = system.get_aux_data().get<sid_t>(payload.sampler_ids);
			const chunk_handle32_t* jsons		  = system.get_aux_data().get<chunk_handle32_t>(jsons_handle);

			for (u32 i = 0; i < payload.count; ++i)
			{
				const char*			 json_text = system.get_aux_data().get<char>(jsons[i]);
				const nlohmann::json embedded  = nlohmann::json::parse(json_text, json_text + jsons[i].size, nullptr, false);
				if (embedded.is_discarded())
				{
					SFG_ERR("failed to parse texture sampler edit json for asset {0}", sampler_ids[i]);
					return false;
				}

				const editor_asset_node_handle_t node = asset_manager.find_asset_node_handle(sampler_ids[i]);
				if (node.is_null())
				{
					SFG_ERR("failed to find texture sampler asset node {0}", sampler_ids[i]);
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
				if (!editor_asset_cooker_t::cook_texture_sampler(asset))
					return false;
				if (!asset_manager.reload_asset_node(node))
					return false;
			}
			return true;
		}

		bool texture_sampler_edit_undo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_texture_sampler_edit_payload_t& payload = system.get_payload_as<editor_command_texture_sampler_edit_payload_t>(command);
			return apply_texture_sampler_jsons(system, payload, payload.previous_jsons);
		}

		bool texture_sampler_edit_redo(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_texture_sampler_edit_payload_t& payload = system.get_payload_as<editor_command_texture_sampler_edit_payload_t>(command);
			return apply_texture_sampler_jsons(system, payload, payload.post_jsons);
		}

		bool texture_sampler_edit_cleanup(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_texture_sampler_edit_payload_t& payload = system.get_payload_as<editor_command_texture_sampler_edit_payload_t>(command);
			free_texture_sampler_edit_payload(system, payload);
			return true;
		}
	}

	bool editor_command_texture_sampler_edit_t::edit(span_t<const sid_t> samplers, span_t<const sampler_desc_t> previous, span_t<const sampler_desc_t> post)
	{
		if (samplers.size == 0 || previous.size != samplers.size || post.size != samplers.size)
			return false;
		if (sampler_descs_equal(previous, post))
			return true;
		SFG_ASSERT(samplers.size <= UINT32_MAX);

		editor_command_system_t& command_system = editor_command_system_t::get();

		editor_command_texture_sampler_edit_payload_t payload = {};
		payload.sampler_ids									  = copy_sampler_ids_to_aux(command_system, samplers);
		payload.previous_jsons								  = copy_sampler_jsons_to_aux(command_system, previous);
		payload.post_jsons									  = copy_sampler_jsons_to_aux(command_system, post);
		payload.count										  = static_cast<u32>(samplers.size);

		const editor_command_issue_desc_t desc{
			.undo		= texture_sampler_edit_undo,
			.redo		= texture_sampler_edit_redo,
			.cleanup	= texture_sampler_edit_cleanup,
			.debug_name = "Texture Sampler Edit",
			.type		= editor_command_type_e::texture_sampler_edit,
			.notify		= false,
		};

		const editor_command_handle_t handle = command_system.issue_command(desc, payload);
		if (handle.is_null())
		{
			SFG_ERR("failed to issue texture sampler edit command");
			return false;
		}

		return true;
	}
}
