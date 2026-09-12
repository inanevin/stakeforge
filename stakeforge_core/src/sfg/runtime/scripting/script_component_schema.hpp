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

#pragma once

#include <sfg/common/size_definitions.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
	struct script_component_field_desc_t
	{
		string_t			   name		   = {};
		sid_t				   field_id	   = 0;
		sid_t				   sub_type_id = 0;
		u32					   offset	   = 0;
		u32					   size		   = 0;
		bitmask32			   flags	   = 0;
		reflected_value_type_e value_type  = reflected_value_type_e::invalid;
	};

	struct script_component_desc_t
	{
		vector_t<script_component_field_desc_t> fields	  = {};
		string_t								name	  = {};
		string_t								full_name = {};
		sid_t									type_id	  = 0;
		u32										size	  = 0;
		u32										alignment = 1;

		const script_component_field_desc_t* find_field(sid_t field_id) const;
		bool								 is_layout_equal(const script_component_desc_t& other) const;
		bool								 is_reflection_equal(const script_component_desc_t& other) const;
	};

	struct script_world_script_desc_t
	{
		string_t name	   = {};
		string_t full_name = {};
		sid_t	 type_id   = 0;
	};

	struct script_component_schema_delta_t
	{
		vector_t<sid_t> added			   = {};
		vector_t<sid_t> removed			   = {};
		vector_t<sid_t> layout_changed	   = {};
		vector_t<sid_t> reflection_changed = {};

		bool has_changes() const;
	};

	class script_component_schema_t final
	{
	public:
		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		bool							parse(const char* schema_json);
		void							register_reflection_types() const;
		script_component_schema_delta_t compare(const script_component_schema_t& candidate) const;
		bool							is_equivalent(const script_component_schema_t& other) const;

		// -----------------------------------------------------------------------------
		// queries
		// -----------------------------------------------------------------------------

		const script_component_desc_t*	  find_component(sid_t type_id) const;
		const script_world_script_desc_t* find_world_script(sid_t type_id) const;

		inline const vector_t<script_component_desc_t>& get_components() const
		{
			return _components;
		}

		inline const vector_t<script_world_script_desc_t>& get_world_scripts() const
		{
			return _world_scripts;
		}

		size_t get_field_count() const;

	private:
		vector_t<script_component_desc_t>	 _components	= {};
		vector_t<script_world_script_desc_t> _world_scripts = {};
	};
}
