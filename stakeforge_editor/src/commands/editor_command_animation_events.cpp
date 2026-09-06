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

#include "editor_command_animation_events.hpp"
#include "editor_command_system.hpp"
#include "ui/panels/editor_panel_animation.hpp"

#include <sfg/common/type_id.hpp>
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
	namespace
	{
		chunk_handle32_t events_to_aux(editor_command_system_t& system, const vector_t<animation_event_def_t>& events)
		{
			ostream_t stream = {};

			stream << static_cast<u32>(events.size());

			for (const animation_event_def_t& event : events)
			{
				if (!reflection_registry_t::get().type_to_stream(type_id_t<animation_event_def_t>::value, const_cast<animation_event_def_t*>(&event), nullptr, stream))
				{
					SFG_ERR("failed to serialize animation event");
					return {};
				}
			}

			const chunk_handle32_t handle = system.get_aux_data().allocate_bytes(stream.get_size(), alignof(u8));

			SFG_MEMCPY(system.get_aux_data().get<u8>(handle), stream.get_raw(), stream.get_size());
			return handle;
		}

		bool events_from_aux(editor_command_system_t& system, editor_panel_animation_t& viewer, chunk_handle32_t handle, u32 selected_event)
		{
			istream_t stream(system.get_aux_data().get<u8>(handle), handle.size);
			u32		  count = 0;

			stream >> count;

			vector_t<animation_event_def_t> events = {};

			events.resize(count);

			for (animation_event_def_t& event : events)
			{
				if (!reflection_registry_t::get().type_from_stream(type_id_t<animation_event_def_t>::value, &event, nullptr, stream))
				{
					SFG_ERR("failed to deserialize animation event");
					return false;
				}
			}

			viewer.apply_events(std::move(events), selected_event);

			return true;
		}

		bool animation_events_edit_undo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_animation_events_edit_payload_t& payload = system.get_payload_as<editor_command_animation_events_edit_payload_t>(command);
			editor_panel_animation_t&							  viewer  = *static_cast<editor_panel_animation_t*>(command.user_data);

			if (!events_from_aux(system, viewer, payload.previous_stream, payload.previous_event))
				return false;

			return true;
		}

		bool animation_events_edit_redo(editor_command_system_t& system, editor_command_t& command)
		{
			const editor_command_animation_events_edit_payload_t& payload = system.get_payload_as<editor_command_animation_events_edit_payload_t>(command);
			editor_panel_animation_t&							  viewer  = *static_cast<editor_panel_animation_t*>(command.user_data);

			if (!events_from_aux(system, viewer, payload.post_stream, payload.post_event))
				return false;

			return true;
		}

		bool animation_events_edit_cleanup(editor_command_system_t& system, editor_command_t& command)
		{
			editor_command_animation_events_edit_payload_t& payload = system.get_payload_as<editor_command_animation_events_edit_payload_t>(command);

			system.get_aux_data().free(payload.previous_stream);
			system.get_aux_data().free(payload.post_stream);
			payload = {};
			return true;
		}
	}

	bool editor_command_animation_events_edit_t::begin(editor_panel_animation_t& viewer)
	{
		SFG_ASSERT(!viewer._edit_previous_stream);

		editor_command_system_t& command_system = *viewer._commands;
		const chunk_handle32_t	 stream			= events_to_aux(command_system, viewer._events);

		if (!stream)
			return false;

		viewer._edit_previous_stream = stream;
		viewer._edit_previous_event	 = viewer._selected_event;

		return true;
	}

	bool editor_command_animation_events_edit_t::submit(editor_panel_animation_t& viewer, const char* debug_name, bool notify)
	{
		SFG_ASSERT(viewer._edit_previous_stream);

		editor_command_system_t& command_system = *viewer._commands;
		const chunk_handle32_t	 post_stream	= events_to_aux(command_system, viewer._events);

		if (!post_stream)
		{
			events_from_aux(command_system, viewer, viewer._edit_previous_stream, viewer._edit_previous_event);
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

		const editor_command_animation_events_edit_payload_t payload{
			.previous_stream = viewer._edit_previous_stream,
			.post_stream	 = post_stream,
			.previous_event	 = viewer._edit_previous_event,
			.post_event		 = viewer._selected_event,
		};

		const editor_command_issue_desc_t desc{
			.undo		= animation_events_edit_undo,
			.redo		= animation_events_edit_redo,
			.cleanup	= animation_events_edit_cleanup,
			.user_data	= &viewer,
			.debug_name = debug_name,
			.type		= editor_command_type_e::animation_events_edit,
			.run_redo	= false,
			.notify		= notify,
		};

		const editor_command_handle_t handle = command_system.issue_command(desc, payload);

		if (handle.is_null())
		{
			events_from_aux(command_system, viewer, viewer._edit_previous_stream, viewer._edit_previous_event);
			command_system.get_aux_data().free(viewer._edit_previous_stream);
			command_system.get_aux_data().free(post_stream);
			viewer._edit_previous_stream = {};
			SFG_ERR("failed to issue animation event edit command");

			return false;
		}

		viewer._edit_previous_stream = {};

		return true;
	}

	void editor_command_animation_events_edit_t::cancel(editor_panel_animation_t& viewer)
	{
		if (!viewer._edit_previous_stream)
			return;

		viewer._commands->get_aux_data().free(viewer._edit_previous_stream);
		viewer._edit_previous_stream = {};
	}
}
