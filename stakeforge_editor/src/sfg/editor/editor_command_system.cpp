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
#include "editor/editor_command_system.hpp"
#include <sfg/core/input/input_mappings.hpp>
#include <sfg/core/io/log.hpp>
#include <sfg/core/platform/common_window.hpp>
#include <sfg/core/platform/process.hpp>

namespace sfg
{
	editor_command_system_t* editor_command_system_t::s_instance = nullptr;

	void editor_command_system_t::init(const editor_command_system_config_t& config)
	{
		SFG_ASSERT(config.command_max_count != 0);
		SFG_ASSERT(config.aux_data_page_size_bytes != 0);

		s_instance = this;
		_config	   = config;

		_commands.reserve(config.command_max_count * 2);
		_listeners.reserve(config.listener_initial_capacity);

		for (history_t& history : _histories)
		{
			history.commands.reserve(config.command_max_count);
			history.cursor = 0;
		}

		_aux_data.init(config.aux_data_page_size_bytes);

		_play_history_active = false;
		_generation			 = 0;
		_entity_generation	 = 0;
	}

	void editor_command_system_t::uninit()
	{
		clear();
		_commands.clear();
		_listeners.clear();

		for (history_t& history : _histories)
			history.commands.clear();

		_aux_data.uninit();

		_config				 = {};
		_play_history_active = false;
		_generation			 = 0;
		_entity_generation	 = 0;

		if (s_instance == this)
			s_instance = nullptr;
	}

	void editor_command_system_t::clear()
	{
		for (history_t& history : _histories)
			clear_history(history);

		_commands.resize_zero();
		_aux_data.reset();

		++_generation;
	}

	void editor_command_system_t::begin_play_history()
	{
		SFG_ASSERT(!_play_history_active);
		SFG_ASSERT(_histories[1].commands.empty());

		_play_history_active = true;
		++_generation;
	}

	void editor_command_system_t::end_play_history()
	{
		SFG_ASSERT(_play_history_active);

		clear_history(_histories[1]);

		_play_history_active = false;
		++_generation;
	}

	void editor_command_system_t::clear_world(editor_world_handle_t world)
	{
		bool removed = false;

		for (history_t& history : _histories)
		{
			size_t write_cursor = 0;
			u32	   cursor		= history.cursor;

			for (size_t read_cursor = 0; read_cursor < history.commands.size(); ++read_cursor)
			{
				const editor_command_handle_t handle  = history.commands[read_cursor];
				editor_command_t&			  command = _commands.get(handle);

				if (command.world == world)
				{
					if (read_cursor < history.cursor)
						--cursor;

					destroy_command(handle);
					removed = true;
					continue;
				}

				history.commands[write_cursor++] = handle;
			}

			history.commands.resize(write_cursor);
			history.cursor = cursor;
		}

		if (removed)
			++_generation;
	}

	void editor_command_system_t::clear_user_data(void* user_data)
	{
		bool removed = false;

		for (history_t& history : _histories)
		{
			size_t write_cursor = 0;
			u32	   cursor		= history.cursor;

			for (size_t read_cursor = 0; read_cursor < history.commands.size(); ++read_cursor)
			{
				const editor_command_handle_t handle  = history.commands[read_cursor];
				editor_command_t&			  command = _commands.get(handle);

				if (command.user_data == user_data)
				{
					if (read_cursor < history.cursor)
						--cursor;

					destroy_command(handle);
					removed = true;
					continue;
				}

				history.commands[write_cursor++] = handle;
			}

			history.commands.resize(write_cursor);
			history.cursor = cursor;
		}

		if (removed)
			++_generation;
	}

	editor_command_handle_t editor_command_system_t::issue_command(const editor_command_issue_desc_t& desc, const void* payload_data)
	{
		history_t& history = _histories[_play_history_active];

		truncate_redo(history);
		trim_history_for_new_command(history);

		const chunk_handle32_t		  payload = copy_payload(desc, payload_data);
		const editor_command_handle_t handle  = _commands.emplace();
		editor_command_t&			  command = _commands.get(handle);

		command.payload			  = payload;
		command.user_data		  = desc.user_data;
		command.debug_name		  = desc.debug_name;
		command.undo			  = desc.undo;
		command.redo			  = desc.redo;
		command.cleanup			  = desc.cleanup;
		command.type			  = desc.type;
		command.world			  = desc.world;
		command.entity_generation = desc.entity_generation;

		if (desc.run_redo && !command.redo(*this, command))
		{
			SFG_ERR("failed to redo issued command {0}", command.debug_name != nullptr ? command.debug_name : "");
			destroy_command(handle);
			return {};
		}

		history.commands.push_back(handle);
		history.cursor = static_cast<u32>(history.commands.size());
		bump_generation(command);

		if (desc.notify)
			notify_listeners(command);

		return handle;
	}

	bool editor_command_system_t::undo()
	{
		if (!can_undo())
			return false;

		history_t&					  history = _histories[_play_history_active];
		const editor_command_handle_t handle  = history.commands[history.cursor - 1];
		editor_command_t&			  command = _commands.get(handle);

		if (!command.undo(*this, command))
		{
			SFG_ERR("failed to undo command {0}", command.debug_name != nullptr ? command.debug_name : "");
			return false;
		}

		--history.cursor;
		bump_generation(command);
		notify_listeners(command);

		return true;
	}

	bool editor_command_system_t::redo()
	{
		if (!can_redo())
			return false;

		history_t&					  history = _histories[_play_history_active];
		const editor_command_handle_t handle  = history.commands[history.cursor];
		editor_command_t&			  command = _commands.get(handle);

		if (!command.redo(*this, command))
		{
			SFG_ERR("failed to redo command {0}", command.debug_name != nullptr ? command.debug_name : "");
			return false;
		}

		++history.cursor;
		bump_generation(command);
		notify_listeners(command);

		return true;
	}

	bool editor_command_system_t::on_window_event(const window_event_t& ev)
	{
		if (ev.type != window_event_type_e::key)
			return false;
		if (ev.sub_type != window_event_sub_type_e::press && ev.sub_type != window_event_sub_type_e::repeat)
			return false;

		const bool ctrl = process::is_key_down(static_cast<u16>(input_code::key_lctrl)) || process::is_key_down(static_cast<u16>(input_code::key_rctrl));
		if (!ctrl)
			return false;

		if (ev.button == static_cast<u16>(input_code::key_z))
		{
			undo();
			return true;
		}

		if (ev.button == static_cast<u16>(input_code::key_r))
		{
			redo();
			return true;
		}

		return false;
	}

	editor_command_listener_handle_t editor_command_system_t::add_listener(editor_command_listener_fn fn, void* user_data)
	{
		SFG_ASSERT(fn != nullptr);

		const editor_command_listener_handle_t handle	= _listeners.emplace();
		editor_command_listener_t&			   listener = _listeners.get(handle);

		listener.fn		   = fn;
		listener.user_data = user_data;

		return handle;
	}

	void editor_command_system_t::remove_listener(editor_command_listener_handle_t handle)
	{
		if (_listeners.is_valid(handle))
			_listeners.remove(handle);
	}

	void editor_command_system_t::clear_history(history_t& history)
	{
		for (const editor_command_handle_t handle : history.commands)
			destroy_command(handle);

		history.commands.resize(0);
		history.cursor = 0;
	}

	void editor_command_system_t::truncate_redo(history_t& history)
	{
		while (history.commands.size() > history.cursor)
		{
			const editor_command_handle_t handle = history.commands.back();

			history.commands.pop_back();
			destroy_command(handle);
		}
	}

	void editor_command_system_t::trim_history_for_new_command(history_t& history)
	{
		if (history.commands.size() < _config.command_max_count)
			return;

		const editor_command_handle_t handle = history.commands.front();

		history.commands.erase(history.commands.begin());
		destroy_command(handle);

		if (history.cursor != 0)
			--history.cursor;
	}

	void editor_command_system_t::destroy_command(editor_command_handle_t handle)
	{
		editor_command_t& command = _commands.get(handle);
		if (command.cleanup != nullptr)
			command.cleanup(*this, command);
		if (command.payload.size != 0)
			_aux_data.free(command.payload);
		_commands.remove(handle);
	}

	void editor_command_system_t::bump_generation(const editor_command_t& command)
	{
		++_generation;
		if (command.entity_generation)
			++_entity_generation;
	}

	void editor_command_system_t::notify_listeners(const editor_command_t& command)
	{
		for (auto it = _listeners.begin_handle(); it != _listeners.end_handle(); ++it)
		{
			const editor_command_listener_handle_t handle	= *it;
			const editor_command_listener_t&	   listener = _listeners.get(handle);

			if (listener.fn != nullptr)
				listener.fn(*this, command, listener.user_data);
		}
	}

	chunk_handle32_t editor_command_system_t::copy_payload(const editor_command_issue_desc_t& desc, const void* payload_data)
	{
		if (desc.payload_size == 0)
			return {};

		const chunk_handle32_t payload = _aux_data.allocate_bytes(desc.payload_size, desc.payload_alignment);

		SFG_MEMCPY(_aux_data.get<u8>(payload), payload_data, desc.payload_size);

		return payload;
	}
}
