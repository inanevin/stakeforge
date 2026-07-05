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

#include <sfg/data/span.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/math/color.hpp>
#include <sfg/memory/gen_pool.hpp>
#include <sfg/runtime/engine/common_engine.hpp>
#include <sfg/runtime/ui/ui_common.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>
#include <sfg/vendor/nhlohmann/json_fwd.hpp>

namespace sfg
{
	class world_t;
	struct ecs_component_table_t;

	struct editor_world_folder_tag_t;
	using editor_world_folder_handle_t = pool_handle_t<u32, editor_world_folder_tag_t>;

	enum class editor_outliner_item_type_e : u8
	{
		entity,
		folder,
	};

	struct editor_world_folder_t
	{
		vector_t<entity_guid_t>		 entity_guids  = {};
		string_t					 name_storage  = {};
		const char*					 name		   = nullptr;
		color_t						 color		   = {};
		u64							 guid		   = 0;
		editor_world_folder_handle_t parent_handle = {};
		bool						 folded		   = false;
	};

	struct editor_world_entity_metadata_t
	{
		editor_world_folder_handle_t folder_handle = {};
		entity_guid_t				 guid		   = NULL_ENTITY_GUID;
		bool						 folded		   = true;
	};

	struct editor_outliner_item_t
	{
		const char*					 name				  = nullptr;
		const char*					 type_icon			  = nullptr;
		entity_id_t					 entity				  = NULL_ENTITY_ID;
		entity_id_t					 parent_entity		  = NULL_ENTITY_ID;
		entity_guid_t				 entity_guid		  = NULL_ENTITY_GUID;
		editor_world_folder_handle_t folder_handle		  = {};
		color_t						 color				  = {};
		u16							 depth				  = 0;
		editor_outliner_item_type_e	 type				  = editor_outliner_item_type_e::entity;
		bool						 has_children		  = false;
		bool						 disabled			  = false;
		bool						 has_prefab_reference = false;
	};

	struct editor_outliner_row_t
	{
		ui::widget_id_t				 root			= NULL_WIDGET;
		ui::widget_id_t				 fold_icon		= NULL_WIDGET;
		ui::widget_id_t				 fold_icon_text = NULL_WIDGET;
		ui::widget_id_t				 type_icon		= NULL_WIDGET;
		ui::widget_id_t				 type_icon_text = NULL_WIDGET;
		ui::widget_id_t				 label			= NULL_WIDGET;
		ui::widget_id_t				 disable_button = NULL_WIDGET;
		ui::widget_id_t				 disable_icon	= NULL_WIDGET;
		entity_id_t					 entity			= NULL_ENTITY_ID;
		editor_world_folder_handle_t folder_handle	= {};
		u16							 depth			= 0;
		editor_outliner_item_type_e	 type			= editor_outliner_item_type_e::entity;
		bool						 has_children	= false;
		bool						 disabled		= false;
	};

	class editor_world_metadata_t final
	{
	public:
		editor_world_metadata_t()										   = default;
		~editor_world_metadata_t()										   = default;
		editor_world_metadata_t(const editor_world_metadata_t&)			   = delete;
		editor_world_metadata_t& operator=(const editor_world_metadata_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init();
		void uninit();
		void set_world(world_handle_t world);

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		editor_world_folder_handle_t create_folder(const char* name);
		editor_world_folder_handle_t create_folder_with_guid(const char* name, color_t color, bool folded, editor_world_folder_handle_t parent_handle, u64 guid);
		void						 destroy_folder(editor_world_folder_handle_t handle);
		void						 set_folder_name(editor_world_folder_handle_t handle, const char* name);
		void						 set_folder_color(editor_world_folder_handle_t handle, color_t color);
		void						 set_folder_folded(editor_world_folder_handle_t handle, bool folded);
		void						 set_folder_parent(editor_world_folder_handle_t handle, editor_world_folder_handle_t parent_handle);
		void						 set_entity_folded(entity_guid_t guid, bool folded);
		void						 assign_entities_to_folder(editor_world_folder_handle_t handle, span_t<const entity_guid_t> entity_guids);
		void						 deassign_entities_from_folder(span_t<const entity_guid_t> entity_guids);
		void						 collect_outliner_items(const world_t& world);
		void						 write_folders_to_json(nlohmann::json& out_json) const;
		void						 read_folders_from_json(const nlohmann::json& in_json);

		// -----------------------------------------------------------------------------
		// queries
		// -----------------------------------------------------------------------------

		editor_world_folder_t&				 get_folder(editor_world_folder_handle_t handle);
		const editor_world_folder_t&		 get_folder(editor_world_folder_handle_t handle) const;
		editor_world_folder_handle_t		 get_folder_handle(u64 guid) const;
		editor_world_folder_handle_t		 get_entity_folder(entity_guid_t guid) const;
		bool								 is_folder_valid(editor_world_folder_handle_t handle) const;
		bool								 can_assign_folder(editor_world_folder_handle_t handle, editor_world_folder_handle_t parent_handle) const;
		bool								 is_entity_expanded(entity_guid_t guid) const;
		void								 collect_folder_tree(editor_world_folder_handle_t handle, vector_t<editor_world_folder_handle_t>& out_handles) const;
		span_t<editor_outliner_item_t>		 get_outliner_items();
		span_t<const editor_outliner_item_t> get_outliner_items() const;
		vector_t<editor_outliner_row_t>&	 get_outliner_rows();
		world_handle_t						 get_world() const;

		static inline editor_world_metadata_t& get()
		{
			SFG_ASSERT(s_instance != nullptr);
			return *s_instance;
		}

	private:
		struct outliner_component_tables_t
		{
			const ecs_component_table_t* hierarchy = nullptr;
			const ecs_component_table_t* name	   = nullptr;
			const ecs_component_table_t* disabled  = nullptr;
			const ecs_component_table_t* prefab	   = nullptr;
		};

		editor_world_entity_metadata_t&		  get_or_create_entity_metadata(entity_guid_t guid);
		editor_world_entity_metadata_t*		  find_entity_metadata(entity_guid_t guid);
		const editor_world_entity_metadata_t* find_entity_metadata(entity_guid_t guid) const;
		void								  append_folder_items(const world_t& world, const outliner_component_tables_t& tables, editor_world_folder_handle_t handle, u16 depth);
		void								  append_entity_items(const world_t& world, const outliner_component_tables_t& tables, entity_id_t id, u16 depth);
		bool								  is_entity_assigned(entity_guid_t guid) const;
		void								  remove_entity_from_folders(entity_guid_t guid);

	private:
		gen_pool_t<editor_world_folder_t, u32, editor_world_folder_tag_t> _folders;
		vector_t<editor_world_entity_metadata_t>						  _entity_metadata;
		vector_t<editor_outliner_item_t>								  _outliner_items;
		vector_t<editor_outliner_row_t>									  _outliner_rows;
		world_handle_t													  _world	 = {};
		u64																  _next_guid = 1;
		bool															  _inited	 = false;

		static inline editor_world_metadata_t* s_instance = nullptr;
	};
}
