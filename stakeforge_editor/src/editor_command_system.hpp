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

#pragma once

#include "world/editor_world_handle.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/memory/chunk_allocator.hpp>
#include <sfg/memory/dynamic_gen_pool.hpp>

namespace sfg
{
	class editor_command_system_t;
	struct window_event_t;

	enum class editor_command_type_e : u16
	{
		invalid,
		custom,
		entity_create,
		entity_selection,
		entity_duplicate,
		entity_destroy,
		entity_reparent,
		prefab_spawn,
		primitive_spawn,
		entity_info_paste,
		entity_info_edit,
		component_add,
		component_remove,
		component_reset,
		component_paste,
		component_edit,
		world_edit_context_create_folder,
		world_edit_context_rename_folder,
		world_edit_context_color_folder,
		world_edit_context_assign_folder,
		world_edit_context_assign_folder_parent,
		world_edit_context_delete_folder,
		project_settings_edit,
		material_edit,
		shader_edit,
		texture_sampler_edit,
		physical_material_edit,
		animation_graph_add_node,
		animation_graph_delete_node,
		animation_graph_duplicate_node,
		animation_graph_select_node,
		animation_graph_make_entry,
		animation_graph_make_exit,
		animation_graph_connect_nodes,
		animation_graph_edit,
		animation_graph_navigation,
	};

	enum class editor_command_state_e : u8
	{
		invalid,
		done,
		undone,
	};

	struct editor_command_tag_t;
	struct editor_command_listener_tag_t;
	using editor_command_handle_t		   = pool_handle_t<u32, editor_command_tag_t>;
	using editor_command_listener_handle_t = pool_handle_t<u32, editor_command_listener_tag_t>;

	struct editor_command_t
	{
		using fn_t = bool (*)(editor_command_system_t& system, editor_command_t& command);

		void*				   user_data		 = nullptr;
		const char*			   debug_name		 = nullptr;
		fn_t				   undo				 = nullptr;
		fn_t				   redo				 = nullptr;
		fn_t				   cleanup			 = nullptr;
		u32					   sequence			 = 0;
		chunk_handle32_t	   payload			 = {};
		editor_world_handle_t  world			 = {};
		editor_command_type_e  type				 = editor_command_type_e::invalid;
		editor_command_state_e state			 = editor_command_state_e::invalid;
		bool				   entity_generation = false;
	};

	struct editor_command_system_config_t
	{
		u32	   max_commands	 = 1024;
		u32	   max_listeners = 256;
		size_t aux_data_size = 4ull * 1024ull * 1024ull;
	};

	using editor_command_listener_fn = void (*)(editor_command_system_t& system, const editor_command_t& command, void* user_data);

	struct editor_command_listener_t
	{
		editor_command_listener_fn fn		 = nullptr;
		void*					   user_data = nullptr;
	};

	struct editor_command_issue_desc_t
	{
		editor_command_t::fn_t undo				 = nullptr;
		editor_command_t::fn_t redo				 = nullptr;
		editor_command_t::fn_t cleanup			 = nullptr;
		void*				   user_data		 = nullptr;
		const char*			   debug_name		 = nullptr;
		size_t				   payload_size		 = 0;
		size_t				   payload_alignment = alignof(std::max_align_t);
		editor_command_type_e  type				 = editor_command_type_e::custom;
		editor_world_handle_t  world			 = {};
		bool				   run_redo			 = true;
		bool				   notify			 = true;
		bool				   entity_generation = false;
	};

	class editor_command_system_t final
	{
	public:
		static constexpr u32	DEFAULT_MAX_COMMANDS  = 1024;
		static constexpr size_t DEFAULT_AUX_DATA_SIZE = 4ull * 1024ull * 1024ull;

		editor_command_system_t()										   = default;
		~editor_command_system_t()										   = default;
		editor_command_system_t(const editor_command_system_t&)			   = delete;
		editor_command_system_t& operator=(const editor_command_system_t&) = delete;

		static inline editor_command_system_t& get()
		{
			return *s_instance;
		}

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(const editor_command_system_config_t& config = {});
		void uninit();
		void clear();
		void clear_world(editor_world_handle_t world);

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		editor_command_handle_t			 issue_command(const editor_command_issue_desc_t& desc, const void* payload_data = nullptr);
		bool							 undo();
		bool							 redo();
		bool							 on_window_event(const window_event_t& ev);
		editor_command_listener_handle_t add_listener(editor_command_listener_fn fn, void* user_data);
		void							 remove_listener(editor_command_listener_handle_t handle);

		template <typename T> editor_command_handle_t issue_command(const editor_command_issue_desc_t& desc, const T& payload)
		{
			static_assert(std::is_trivially_copyable_v<T>);
			static_assert(std::is_trivially_destructible_v<T>);

			editor_command_issue_desc_t typed_desc = desc;
			typed_desc.payload_size				   = sizeof(T);
			typed_desc.payload_alignment		   = alignof(T);
			return issue_command(typed_desc, static_cast<const void*>(&payload));
		}

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline editor_command_t& get_command(editor_command_handle_t handle)
		{
			return _commands.get(handle);
		}

		inline const editor_command_t& get_command(editor_command_handle_t handle) const
		{
			return _commands.get(handle);
		}

		inline void* get_payload(editor_command_t& command)
		{
			if (command.payload.size == 0)
				return nullptr;
			return _aux_data.get<u8>(command.payload);
		}

		inline const void* get_payload(const editor_command_t& command) const
		{
			if (command.payload.size == 0)
				return nullptr;
			return _aux_data.get<u8>(command.payload);
		}

		inline chunk_allocator_t& get_aux_data()
		{
			return _aux_data;
		}

		inline const chunk_allocator_t& get_aux_data() const
		{
			return _aux_data;
		}

		inline bool is_valid(editor_command_handle_t handle) const
		{
			return _commands.is_valid(handle);
		}

		inline bool can_undo() const
		{
			return _cursor != 0;
		}

		inline bool can_redo() const
		{
			return _cursor < _history.size();
		}

		inline u32 get_history_size() const
		{
			return static_cast<u32>(_history.size());
		}

		inline u32 get_history_cursor() const
		{
			return _cursor;
		}

		inline u32 get_generation() const
		{
			return _generation;
		}

		inline u32 get_entity_generation() const
		{
			return _entity_generation;
		}

		inline bool is_listener_valid(editor_command_listener_handle_t handle) const
		{
			return _listeners.is_valid(handle);
		}

		template <typename T> T& get_payload_as(editor_command_t& command)
		{
			static_assert(std::is_trivially_copyable_v<T>);
			return *static_cast<T*>(get_payload(command));
		}

		template <typename T> const T& get_payload_as(const editor_command_t& command) const
		{
			static_assert(std::is_trivially_copyable_v<T>);
			return *static_cast<const T*>(get_payload(command));
		}

	private:
		void			 truncate_redo();
		void			 trim_history_for_new_command();
		void			 destroy_command(editor_command_handle_t handle);
		void			 bump_generation(const editor_command_t& command);
		void			 notify_listeners(const editor_command_t& command);
		chunk_handle32_t copy_payload(const editor_command_issue_desc_t& desc, const void* payload_data);

	private:
		dynamic_gen_pool_t<editor_command_t, u32, editor_command_tag_t>					  _commands;
		dynamic_gen_pool_t<editor_command_listener_t, u32, editor_command_listener_tag_t> _listeners;
		vector_t<editor_command_handle_t>												  _history;
		chunk_allocator_t																  _aux_data;
		editor_command_system_config_t													  _config			 = {};
		u32																				  _cursor			 = 0;
		u32																				  _next_sequence	 = 1;
		u32																				  _generation		 = 0;
		u32																				  _entity_generation = 0;

		static inline editor_command_system_t* s_instance = nullptr;
	};
}
