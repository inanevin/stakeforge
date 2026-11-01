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

#include "core/data/string.hpp"
#include "core/reflection/reflection_registry.hpp"

namespace sfg
{
	struct plugin_type_schema_delta_t
	{
		vector_t<sid_t> added		   = {};
		vector_t<sid_t> removed		   = {};
		vector_t<sid_t> layout_changed = {};
	};

	class SFG_API plugin_type_schema_t final
	{
	public:
		plugin_type_schema_t() = default;
		~plugin_type_schema_t();
		plugin_type_schema_t(const plugin_type_schema_t& other)			   = delete;
		plugin_type_schema_t& operator=(const plugin_type_schema_t& other) = delete;

		explicit plugin_type_schema_t(reflection_owner_e owner, reflection_owner_e dependency_owner = reflection_owner_e::engine) : _owner(owner), _dependency_owner(dependency_owner)
		{
		}

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		template <typename T> void register_type(reflected_type_descriptor_t descriptor)
		{
			static_assert(std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>);

			descriptor.type_id		   = type_id_t<T>::value;
			descriptor.size			   = sizeof(T);
			descriptor.alignment	   = alignof(T);
			descriptor.owner		   = _owner;
			descriptor.default_init_fn = [](void* obj) { std::construct_at(static_cast<T*>(obj), T{}); };

			add_type(std::move(descriptor));
		}

		bool					   validate(string_t& diagnostics) const;
		void					   register_reflection_types() const;
		plugin_type_schema_delta_t compare(const plugin_type_schema_t& candidate) const;
		void					   migrate_type(const plugin_type_schema_t& current, sid_t type_id, const void* source, void* destination) const;

		// -----------------------------------------------------------------------------
		// queries
		// -----------------------------------------------------------------------------

		const reflected_type_descriptor_t* find_type(sid_t type_id) const;

		const vector_t<reflected_type_descriptor_t>& get_types() const
		{
			return _types;
		}

	private:
		void add_type(reflected_type_descriptor_t descriptor);
		bool is_layout_equal(const plugin_type_schema_t& candidate, sid_t type_id) const;
		bool is_field_valid(const reflected_type_descriptor_t& type, const reflected_field_descriptor_t& field, size_t depth, string_t& diagnostics) const;
		bool is_type_valid(const reflected_type_descriptor_t& type, size_t depth, string_t& diagnostics) const;

		vector_t<reflected_type_descriptor_t> _types			= {};
		text_allocator_t					  _text_allocator	= {};
		size_t								  _field_count		= 0;
		bool								  _valid			= true;
		reflection_owner_e					  _owner			= reflection_owner_e::engine;
		reflection_owner_e					  _dependency_owner = reflection_owner_e::engine;
	};
}
