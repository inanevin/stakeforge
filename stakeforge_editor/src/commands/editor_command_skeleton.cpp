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

#include "commands/editor_command_skeleton.hpp"
#include "assets/editor_asset_manager.hpp"
#include "editor_command_system.hpp"
#include "ui/panels/editor_panel_skeleton_viewer.hpp"

#include <sfg/common/type_id.hpp>
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	namespace
	{
		bool save_and_cook_skeleton_async(editor_panel_skeleton_viewer_t& viewer)
		{
			nlohmann::json	embedded_source = nlohmann::json::object();
			skeleton_def_t& skeleton		= viewer.get_skeleton_def();

			if (!reflection_registry_t::get().type_to_json(type_id_t<skeleton_def_t>::value, &skeleton, nullptr, embedded_source))
			{
				SFG_ERR("failed to serialize skeleton definition for asset {0}", viewer.get_skeleton_guid());
				return false;
			}

			return editor_asset_manager_t::get().save_and_cook_embedded_asset_async(viewer.get_skeleton_guid(), embedded_source);
		}

		chunk_handle32_t skeleton_to_aux(editor_command_system_t& system, const skeleton_def_t& skeleton)
		{
			ostream_t stream = {};

			if (!reflection_registry_t::get().type_to_stream(type_id_t<skeleton_def_t>::value, const_cast<skeleton_def_t*>(&skeleton), nullptr, stream))
			{
				SFG_ERR("failed to serialize skeleton");
				return {};
			}

			const chunk_handle32_t handle = system.get_aux_data().allocate_bytes(stream.get_size(), alignof(u8));

			SFG_MEMCPY(system.get_aux_data().get<u8>(handle), stream.get_raw(), stream.get_size());
			return handle;
		}

		bool skeleton_from_aux(editor_command_system_t& system, editor_panel_skeleton_viewer_t& viewer, chunk_handle32_t handle)
		{
			skeleton_def_t skeleton = {};
			istream_t	   stream(system.get_aux_data().get<u8>(handle), handle.size);

			if (!reflection_registry_t::get().type_from_stream(type_id_t<skeleton_def_t>::value, &skeleton, nullptr, stream))
			{
				SFG_ERR("failed to deserialize skeleton");
				return false;
			}

			viewer.apply_skeleton_def(std::move(skeleton));
			return true;
		}

		bool skeleton_edit_undo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_skeleton_edit_payload_t& payload = system.get_payload_as<editor_command_skeleton_edit_payload_t>(command);
			editor_panel_skeleton_viewer_t&				  viewer  = *static_cast<editor_panel_skeleton_viewer_t*>(command.user_data);

			if (!skeleton_from_aux(system, viewer, payload.previous_stream))
				return false;

			save_and_cook_skeleton_async(viewer);
			return true;
		}

		bool skeleton_edit_redo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_skeleton_edit_payload_t& payload = system.get_payload_as<editor_command_skeleton_edit_payload_t>(command);
			editor_panel_skeleton_viewer_t&				  viewer  = *static_cast<editor_panel_skeleton_viewer_t*>(command.user_data);

			if (!skeleton_from_aux(system, viewer, payload.post_stream))
				return false;

			save_and_cook_skeleton_async(viewer);
			return true;
		}

		bool skeleton_edit_cleanup(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_skeleton_edit_payload_t& payload = system.get_payload_as<editor_command_skeleton_edit_payload_t>(command);

			system.get_aux_data().free(payload.previous_stream);
			system.get_aux_data().free(payload.post_stream);
			payload = {};
			return true;
		}
	}

	bool editor_command_skeleton_edit_t::begin(editor_panel_skeleton_viewer_t& viewer)
	{
		SFG_ASSERT(!viewer._edit_previous_stream);

		editor_command_system_t& command_system = editor_command_system_t::get();
		const chunk_handle32_t	 stream			= skeleton_to_aux(command_system, viewer.get_skeleton_def());

		if (!stream)
			return false;

		viewer._edit_previous_stream = stream;
		return true;
	}

	bool editor_command_skeleton_edit_t::submit(editor_panel_skeleton_viewer_t& viewer, const char* debug_name, bool notify)
	{
		SFG_ASSERT(viewer._edit_previous_stream);

		editor_command_system_t& command_system = editor_command_system_t::get();
		const chunk_handle32_t	 post_stream	= skeleton_to_aux(command_system, viewer.get_skeleton_def());

		if (!post_stream)
		{
			skeleton_from_aux(command_system, viewer, viewer._edit_previous_stream);
			command_system.get_aux_data().free(viewer._edit_previous_stream);
			viewer._edit_previous_stream = {};
			return false;
		}

		const bool is_same = viewer._edit_previous_stream.size == post_stream.size && SFG_MEMCMP(command_system.get_aux_data().get<u8>(viewer._edit_previous_stream), command_system.get_aux_data().get<u8>(post_stream), post_stream.size) == 0;

		if (is_same)
		{
			command_system.get_aux_data().free(viewer._edit_previous_stream);
			command_system.get_aux_data().free(post_stream);
			viewer._edit_previous_stream = {};
			return true;
		}

		const editor_command_skeleton_edit_payload_t payload{
			.previous_stream = viewer._edit_previous_stream,
			.post_stream	 = post_stream,
		};
		const editor_command_issue_desc_t desc{
			.undo		= skeleton_edit_undo,
			.redo		= skeleton_edit_redo,
			.cleanup	= skeleton_edit_cleanup,
			.user_data	= &viewer,
			.debug_name = debug_name,
			.type		= editor_command_type_e::skeleton_edit,
			.run_redo	= false,
			.notify		= notify,
		};
		const editor_command_handle_t handle = command_system.issue_command(desc, payload);

		if (handle.is_null())
		{
			skeleton_from_aux(command_system, viewer, viewer._edit_previous_stream);
			command_system.get_aux_data().free(viewer._edit_previous_stream);
			command_system.get_aux_data().free(post_stream);
			viewer._edit_previous_stream = {};
			SFG_ERR("failed to issue skeleton edit command");
			return false;
		}

		viewer._edit_previous_stream = {};
		save_and_cook_skeleton_async(viewer);
		return true;
	}

	void editor_command_skeleton_edit_t::cancel(editor_panel_skeleton_viewer_t& viewer)
	{
		if (!viewer._edit_previous_stream)
			return;

		editor_command_system_t::get().get_aux_data().free(viewer._edit_previous_stream);
		viewer._edit_previous_stream = {};
	}
}
