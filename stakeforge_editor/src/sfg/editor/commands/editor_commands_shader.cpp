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

#include "editor_commands_shader.hpp"
#include "editor/assets/editor_asset_io.hpp"
#include "editor/assets/editor_asset_manager.hpp"
#include "editor/editor_command_system.hpp"

#include <sfg/core/data/frame_vector.hpp>
#include <sfg/core/data/istream.hpp>
#include <sfg/core/data/ostream.hpp>
#include <sfg/core/io/log.hpp>
#include <sfg/core/memory/memory.hpp>
#include <sfg/core/runtime/resources/shader_cook.hpp>
#include <sfg/core/vendor/nhlohmann/json.hpp>

namespace sfg
{
	namespace
	{
		struct shader_options_edit_payload_t
		{
			chunk_handle32_t names[2] = {};
		};

		bool apply_shader_type_names(editor_command_system_t& system, chunk_handle32_t names)
		{
			struct pending_edit_t
			{
				nlohmann::json options = {};
				sid_t		   id	   = NULL_SID;
			};

			istream_t stream = {};
			stream.open(system.get_aux_data().get<u8>(names), names.size);

			u32 count = 0;
			stream >> count;

			frame_vector_t<pending_edit_t> pending = {};
			pending.reserve(count);

			editor_asset_manager_t& assets = editor_asset_manager_t::get();

			for (u32 i = 0; i < count; ++i)
			{
				pending_edit_t edit		 = {};
				string_t	   type_name = {};
				stream >> edit.id >> type_name;

				const editor_asset_t* asset = assets.find_asset(edit.id);

				if (asset == nullptr)
					return false;

				const bool parsed = editor_asset_io_t::parse_cook_options_json(*asset, edit.options);

				if (!parsed)
					return false;

				if (!edit.options.is_object())
				{
					SFG_ERR("invalid shader cook options for asset {0}", edit.id);
					return false;
				}

				edit.options["custom_type_name"] = type_name;
				pending.push_back(std::move(edit));
			}

			for (const pending_edit_t& edit : pending)
			{
				const bool saved = assets.save_and_cook_file_asset_options_async(edit.id, edit.options);

				if (!saved)
					return false;
			}

			return true;
		}

		bool shader_options_edit_undo(editor_command_system_t& system, editor_command_t& command)
		{
			const shader_options_edit_payload_t& payload = system.get_payload_as<shader_options_edit_payload_t>(command);

			return apply_shader_type_names(system, payload.names[0]);
		}

		bool shader_options_edit_redo(editor_command_system_t& system, editor_command_t& command)
		{
			const shader_options_edit_payload_t& payload = system.get_payload_as<shader_options_edit_payload_t>(command);

			return apply_shader_type_names(system, payload.names[1]);
		}

		bool shader_options_edit_cleanup(editor_command_system_t& system, editor_command_t& command)
		{
			shader_options_edit_payload_t& payload = system.get_payload_as<shader_options_edit_payload_t>(command);

			for (chunk_handle32_t& names : payload.names)
			{
				system.get_aux_data().free(names);
				names = {};
			}

			return true;
		}
	}

	bool editor_command_shader_options_edit_t::edit(span_t<const sid_t> shaders, span_t<const shader_cook_config_t> previous, span_t<const shader_cook_config_t> post)
	{
		SFG_ASSERT(shaders.size == previous.size && shaders.size == post.size);

		bool changed = false;

		for (size_t i = 0; i < shaders.size; ++i)
		{
			if (previous.data[i].custom_type_name != post.data[i].custom_type_name)
				changed = true;
		}

		if (!changed)
			return true;

		editor_command_system_t&				 system		= editor_command_system_t::get();
		shader_options_edit_payload_t			 payload	= {};
		const span_t<const shader_cook_config_t> versions[] = {previous, post};

		for (size_t version = 0; version < std::size(versions); ++version)
		{
			ostream_t stream = {};
			stream << static_cast<u32>(shaders.size);

			for (size_t i = 0; i < shaders.size; ++i)
				stream << shaders.data[i] << versions[version].data[i].custom_type_name;

			payload.names[version] = system.get_aux_data().allocate_bytes(stream.get_size(), alignof(char));
			SFG_MEMCPY(system.get_aux_data().get<u8>(payload.names[version]), stream.get_raw(), stream.get_size());
		}

		const editor_command_issue_desc_t desc{
			.undo		= shader_options_edit_undo,
			.redo		= shader_options_edit_redo,
			.cleanup	= shader_options_edit_cleanup,
			.debug_name = "Shader Type Name Edit",
			.type		= editor_command_type_e::shader_options_edit,
			.notify		= false,
		};
		const editor_command_handle_t handle = system.issue_command(desc, payload);

		if (handle.is_null())
		{
			SFG_ERR("failed to issue shader type name edit command");
			return false;
		}

		return true;
	}
}
