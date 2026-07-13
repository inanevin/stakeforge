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
#include "editor_command_system.hpp"
#include <sfg/input/input_mappings.hpp>
#include <sfg/io/log.hpp>
#include <sfg/platform/common_window.hpp>
#include <sfg/platform/process.hpp>

namespace sfg
{
	void editor_command_system_t::init(const editor_command_system_config_t& config)
	{
		SFG_ASSERT(s_instance == nullptr);
		SFG_ASSERT(!_inited);
		SFG_ASSERT(config.max_commands != 0);
		SFG_ASSERT(config.max_listeners != 0);
		SFG_ASSERT(config.aux_data_size != 0);

		s_instance = this;
		_config	   = config;
		_commands.reserve(config.max_commands);
		_listeners.reserve(config.max_listeners);
		_history.reserve(config.max_commands);
		_aux_data.init(config.aux_data_size);
		_cursor			   = 0;
		_next_sequence	   = 1;
		_generation		   = 0;
		_entity_generation = 0;
		_inited			   = true;
	}

	void editor_command_system_t::uninit()
	{
		SFG_ASSERT(s_instance == this);
		SFG_ASSERT(_inited);
		clear();
		_commands.clear();
		_listeners.clear();
		_history.clear();
		_aux_data.uninit();
		_config			   = {};
		_cursor			   = 0;
		_next_sequence	   = 1;
		_generation		   = 0;
		_entity_generation = 0;
		_inited			   = false;
		s_instance		   = nullptr;
	}

	void editor_command_system_t::clear()
	{
		SFG_ASSERT(_inited);

		for (const editor_command_handle_t handle : _history)
		{
			if (_commands.is_valid(handle))
				destroy_command(handle);
		}

		_history.resize(0);
		_commands.resize_zero();
		_aux_data.reset();
		_cursor		   = 0;
		_next_sequence = 1;
		++_generation;
	}

	void editor_command_system_t::clear_world(editor_world_handle_t world)
	{
		SFG_ASSERT(_inited);
		SFG_ASSERT(!world.is_null());

		size_t write_cursor = 0;
		u32	   cursor		= _cursor;
		bool   removed		= false;
		for (size_t read_cursor = 0; read_cursor < _history.size(); ++read_cursor)
		{
			const editor_command_handle_t handle  = _history[read_cursor];
			editor_command_t&			  command = _commands.get(handle);
			if (command.world == world)
			{
				if (read_cursor < _cursor)
					--cursor;
				destroy_command(handle);
				removed = true;
				continue;
			}

			_history[write_cursor++] = handle;
		}

		_history.resize(write_cursor);
		_cursor = cursor;
		if (removed)
			++_generation;
	}

	editor_command_handle_t editor_command_system_t::issue_command(const editor_command_issue_desc_t& desc, const void* payload_data)
	{
		SFG_ASSERT(_inited);
		SFG_ASSERT(desc.type != editor_command_type_e::invalid);
		SFG_ASSERT(desc.undo != nullptr);
		SFG_ASSERT(desc.redo != nullptr);
		SFG_ASSERT(desc.payload_size == 0 || payload_data != nullptr);

		truncate_redo();
		trim_history_for_new_command();

		const chunk_handle32_t		  payload = copy_payload(desc, payload_data);
		const editor_command_handle_t handle  = _commands.emplace();
		editor_command_t&			  command = _commands.get(handle);
		command.payload						  = payload;
		command.user_data					  = desc.user_data;
		command.debug_name					  = desc.debug_name;
		command.undo						  = desc.undo;
		command.redo						  = desc.redo;
		command.cleanup						  = desc.cleanup;
		command.sequence					  = _next_sequence++;
		command.type						  = desc.type;
		command.world						  = desc.world;
		command.state						  = editor_command_state_e::done;
		command.entity_generation			  = desc.entity_generation;

		if (desc.run_redo && !command.redo(*this, command))
		{
			SFG_ERR("failed to redo issued command {0}", command.debug_name != nullptr ? command.debug_name : "");
			destroy_command(handle);
			return {};
		}

		_history.push_back(handle);
		_cursor = static_cast<u32>(_history.size());
		bump_generation(command);
		if (desc.notify)
			notify_listeners(command);
		return handle;
	}

	bool editor_command_system_t::undo()
	{
		SFG_ASSERT(_inited);
		if (!can_undo())
			return false;

		const editor_command_handle_t handle  = _history[_cursor - 1];
		editor_command_t&			  command = _commands.get(handle);
		if (!command.undo(*this, command))
		{
			SFG_ERR("failed to undo command {0}", command.debug_name != nullptr ? command.debug_name : "");
			return false;
		}

		command.state = editor_command_state_e::undone;
		--_cursor;
		bump_generation(command);
		notify_listeners(command);
		return true;
	}

	bool editor_command_system_t::redo()
	{
		SFG_ASSERT(_inited);
		if (!can_redo())
			return false;

		const editor_command_handle_t handle  = _history[_cursor];
		editor_command_t&			  command = _commands.get(handle);
		if (!command.redo(*this, command))
		{
			SFG_ERR("failed to redo command {0}", command.debug_name != nullptr ? command.debug_name : "");
			return false;
		}

		command.state = editor_command_state_e::done;
		++_cursor;
		bump_generation(command);
		notify_listeners(command);
		return true;
	}

	bool editor_command_system_t::on_window_event(const window_event_t& ev)
	{
		SFG_ASSERT(_inited);
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
		SFG_ASSERT(_inited);
		SFG_ASSERT(fn != nullptr);
		const editor_command_listener_handle_t handle	= _listeners.emplace();
		editor_command_listener_t&			   listener = _listeners.get(handle);
		listener.fn										= fn;
		listener.user_data								= user_data;
		return handle;
	}

	void editor_command_system_t::remove_listener(editor_command_listener_handle_t handle)
	{
		SFG_ASSERT(_inited);
		if (_listeners.is_valid(handle))
			_listeners.remove(handle);
	}

	editor_command_t& editor_command_system_t::get_command(editor_command_handle_t handle)
	{
		SFG_ASSERT(_inited);
		return _commands.get(handle);
	}

	const editor_command_t& editor_command_system_t::get_command(editor_command_handle_t handle) const
	{
		SFG_ASSERT(_inited);
		return _commands.get(handle);
	}

	void* editor_command_system_t::get_payload(editor_command_t& command)
	{
		SFG_ASSERT(_inited);
		if (command.payload.size == 0)
			return nullptr;
		return _aux_data.get<u8>(command.payload);
	}

	const void* editor_command_system_t::get_payload(const editor_command_t& command) const
	{
		SFG_ASSERT(_inited);
		if (command.payload.size == 0)
			return nullptr;
		return _aux_data.get<u8>(command.payload);
	}

	chunk_allocator_t& editor_command_system_t::get_aux_data()
	{
		SFG_ASSERT(_inited);
		return _aux_data;
	}

	const chunk_allocator_t& editor_command_system_t::get_aux_data() const
	{
		SFG_ASSERT(_inited);
		return _aux_data;
	}

	bool editor_command_system_t::is_valid(editor_command_handle_t handle) const
	{
		SFG_ASSERT(_inited);
		return _commands.is_valid(handle);
	}

	bool editor_command_system_t::can_undo() const
	{
		SFG_ASSERT(_inited);
		return _cursor != 0;
	}

	bool editor_command_system_t::can_redo() const
	{
		SFG_ASSERT(_inited);
		return _cursor < _history.size();
	}

	u32 editor_command_system_t::get_history_size() const
	{
		SFG_ASSERT(_inited);
		return static_cast<u32>(_history.size());
	}

	u32 editor_command_system_t::get_history_cursor() const
	{
		SFG_ASSERT(_inited);
		return _cursor;
	}

	u32 editor_command_system_t::get_generation() const
	{
		SFG_ASSERT(_inited);
		return _generation;
	}

	u32 editor_command_system_t::get_entity_generation() const
	{
		SFG_ASSERT(_inited);
		return _entity_generation;
	}

	bool editor_command_system_t::is_listener_valid(editor_command_listener_handle_t handle) const
	{
		SFG_ASSERT(_inited);
		return _listeners.is_valid(handle);
	}

	void editor_command_system_t::truncate_redo()
	{
		while (_history.size() > _cursor)
		{
			const editor_command_handle_t handle = _history.back();
			_history.pop_back();
			destroy_command(handle);
		}
	}

	void editor_command_system_t::trim_history_for_new_command()
	{
		if (_history.size() < _config.max_commands)
			return;

		const editor_command_handle_t handle = _history.front();
		_history.erase(_history.begin());
		destroy_command(handle);
		if (_cursor != 0)
			--_cursor;
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
		SFG_MEMCPY(_aux_data.get(payload.head), payload_data, desc.payload_size);
		return payload;
	}
}
