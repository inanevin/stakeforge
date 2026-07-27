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

#include "commands/editor_commands_curve.hpp"
#include "assets/editor_asset_manager.hpp"
#include "editor_command_system.hpp"
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/resources/curve_def.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	namespace
	{
		nlohmann::json curve_def_to_json(const curve_def_t& curve)
		{
			nlohmann::json json = nlohmann::json::object();

			if (!reflection_registry_t::get().type_to_json(type_id_t<curve_def_t>::value, const_cast<curve_def_t*>(&curve), nullptr, json))
				return nlohmann::json::object();

			json["schema"] = "sfg.schema.curve";
			return json;
		}

		chunk_handle32_t copy_curve_ids(editor_command_system_t& system, span_t<const sid_t> curves)
		{
			sid_t*				   dst	  = nullptr;
			const chunk_handle32_t handle = system.get_aux_data().allocate<sid_t>(curves.size, dst);
			SFG_MEMCPY(dst, curves.data, sizeof(sid_t) * curves.size);
			return handle;
		}

		chunk_handle32_t copy_json(editor_command_system_t& system, const nlohmann::json& json)
		{
			const string_t		   text	  = json.dump();
			const chunk_handle32_t handle = system.get_aux_data().allocate_bytes(text.size(), alignof(char));
			SFG_MEMCPY(system.get_aux_data().get<char>(handle), text.data(), text.size());
			return handle;
		}

		chunk_handle32_t copy_curve_jsons(editor_command_system_t& system, span_t<const curve_def_t> curves)
		{
			const chunk_handle32_t handle = system.get_aux_data().allocate_bytes(sizeof(chunk_handle32_t) * curves.size, alignof(chunk_handle32_t));
			chunk_handle32_t*	   jsons  = system.get_aux_data().get<chunk_handle32_t>(handle);

			for (size_t index = 0; index < curves.size; ++index)
				jsons[index] = copy_json(system, curve_def_to_json(curves.data[index]));

			return handle;
		}

		void free_curve_jsons(editor_command_system_t& system, chunk_handle32_t handle, u32 count)
		{
			chunk_handle32_t* jsons = system.get_aux_data().get<chunk_handle32_t>(handle);

			for (u32 index = 0; index < count; ++index)
				system.get_aux_data().free(jsons[index]);

			system.get_aux_data().free(handle);
		}

		bool apply_curve_jsons(editor_command_system_t& system, const editor_command_curve_edit_payload_t& payload, chunk_handle32_t jsons_handle)
		{
			editor_asset_manager_t& assets	  = editor_asset_manager_t::get();
			const sid_t*			curve_ids = system.get_aux_data().get<sid_t>(payload.curve_ids);
			const chunk_handle32_t* jsons	  = system.get_aux_data().get<chunk_handle32_t>(jsons_handle);

			for (u32 index = 0; index < payload.count; ++index)
			{
				const char*			 text	  = system.get_aux_data().get<char>(jsons[index]);
				const nlohmann::json embedded = nlohmann::json::parse(text, text + jsons[index].size, nullptr, false);

				if (embedded.is_discarded())
				{
					SFG_ERR("failed to parse curve edit json for asset {0}", curve_ids[index]);
					return false;
				}

				if (!assets.save_and_cook_embedded_asset_async(curve_ids[index], embedded))
					return false;
			}

			return true;
		}

		bool curve_edit_undo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_curve_edit_payload_t& payload = system.get_payload_as<editor_command_curve_edit_payload_t>(command);
			return apply_curve_jsons(system, payload, payload.previous_jsons);
		}

		bool curve_edit_redo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_curve_edit_payload_t& payload = system.get_payload_as<editor_command_curve_edit_payload_t>(command);
			return apply_curve_jsons(system, payload, payload.post_jsons);
		}

		bool curve_edit_cleanup(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_curve_edit_payload_t& payload = system.get_payload_as<editor_command_curve_edit_payload_t>(command);
			free_curve_jsons(system, payload.previous_jsons, payload.count);
			free_curve_jsons(system, payload.post_jsons, payload.count);
			system.get_aux_data().free(payload.curve_ids);
			payload = {};
			return true;
		}
	}

	bool editor_command_curve_edit_t::edit(span_t<const sid_t> curves, span_t<const curve_def_t> previous, span_t<const curve_def_t> post)
	{
		if (curves.size == 0 || previous.size != curves.size || post.size != curves.size)
			return false;

		bool equal = true;

		for (size_t index = 0; index < previous.size; ++index)
		{
			if (previous.data[index] != post.data[index])
			{
				equal = false;
				break;
			}
		}

		if (equal)
			return true;

		editor_command_system_t&				  system = editor_command_system_t::get();
		const editor_command_curve_edit_payload_t payload{
			.curve_ids		= copy_curve_ids(system, curves),
			.previous_jsons = copy_curve_jsons(system, previous),
			.post_jsons		= copy_curve_jsons(system, post),
			.count			= static_cast<u32>(curves.size),
		};
		const editor_command_issue_desc_t desc{
			.undo		= curve_edit_undo,
			.redo		= curve_edit_redo,
			.cleanup	= curve_edit_cleanup,
			.debug_name = "Curve Edit",
			.type		= editor_command_type_e::curve_edit,
			.notify		= false,
		};

		const editor_command_handle_t handle = system.issue_command(desc, payload);

		if (handle.is_null())
		{
			SFG_ERR("failed to issue curve edit command");
			return false;
		}

		return true;
	}
}
