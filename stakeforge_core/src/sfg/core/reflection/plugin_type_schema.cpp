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

#include "core/reflection/plugin_type_schema.hpp"

#include "core/io/log.hpp"
#include "core/memory/memory.hpp"

namespace sfg
{
#define PLUGIN_COMPONENT_FLAGS (reflected_type_flag_component | reflected_type_flag_system_component | reflected_type_flag_tag_component)

	plugin_type_schema_t::~plugin_type_schema_t()
	{
		_text_allocator.uninit();
	}

	void plugin_type_schema_t::add_type(reflected_type_descriptor_t descriptor)
	{
		size_t	   text_size  = 0;
		const auto count_text = [&](const char* text) {
			if (text != nullptr)
				text_size += std::strlen(text) + 1 + sizeof(u32);
		};

		count_text(descriptor.name);
		count_text(descriptor.display_name);
		count_text(descriptor.category);
		count_text(descriptor.tooltip);

		for (const reflected_field_descriptor_t& field : descriptor.fields)
		{
			count_text(field.name);
			count_text(field.display_name);
			count_text(field.tooltip);
		}

		if (_types.size() >= reflection_registry_t::PLUGIN_TYPE_CAPACITY || _field_count + descriptor.fields.size() > reflection_registry_t::PLUGIN_FIELD_CAPACITY || _text_allocator.get_head() + text_size > reflection_registry_t::PLUGIN_TEXT_CAPACITY)
		{
			_valid = false;
			return;
		}

		if (_text_allocator.get_capacity() == 0)
			_text_allocator.init(reflection_registry_t::PLUGIN_TEXT_CAPACITY);

		descriptor.name			= descriptor.name == nullptr ? nullptr : _text_allocator.allocate(descriptor.name);
		descriptor.display_name = descriptor.display_name == nullptr ? nullptr : _text_allocator.allocate(descriptor.display_name);
		descriptor.category		= descriptor.category == nullptr ? nullptr : _text_allocator.allocate(descriptor.category);
		descriptor.tooltip		= descriptor.tooltip == nullptr ? nullptr : _text_allocator.allocate(descriptor.tooltip);

		for (reflected_field_descriptor_t& field : descriptor.fields)
		{
			field.name		   = field.name == nullptr ? nullptr : _text_allocator.allocate(field.name);
			field.display_name = field.display_name == nullptr ? nullptr : _text_allocator.allocate(field.display_name);
			field.tooltip	   = field.tooltip == nullptr ? nullptr : _text_allocator.allocate(field.tooltip);

			if (field.field_identifier == 0 && field.name != nullptr)
				field.field_identifier = TO_SID(field.name);
		}

		_field_count += descriptor.fields.size();
		_types.push_back(std::move(descriptor));
	}

	const reflected_type_descriptor_t* plugin_type_schema_t::find_type(sid_t type_id) const
	{
		const auto it = std::find_if(_types.begin(), _types.end(), [type_id](const reflected_type_descriptor_t& type) { return type.type_id == type_id; });

		return it == _types.end() ? nullptr : &*it;
	}

	bool plugin_type_schema_t::validate(string_t& diagnostics) const
	{
		if (!_valid)
		{
			diagnostics = "Plugin type reflection capacity exceeded.";
			SFG_ERR("{0}", diagnostics);
			return false;
		}

		for (const reflected_type_descriptor_t& type : _types)
		{
			const reflected_type_t* existing		  = reflection_registry_t::get().find_type(type.type_id);
			const bool				valid_name		  = type.name != nullptr && type.name[0] != '\0';
			const bool				valid_id		  = type.type_id != 0 && type.type_id != NULL_SID;
			const bool				unique_id		  = find_type(type.type_id) == &type;
			const bool				conflicting_owner = existing != nullptr && existing->owner != reflection_owner_e::game_plugin;

			if (!valid_name || !valid_id || !unique_id || conflicting_owner)
			{
				diagnostics = "Plugin reflection contains an invalid or conflicting type ID.";
				SFG_ERR("{0}", diagnostics);
				return false;
			}

			if (!is_type_valid(type, 0, diagnostics))
			{
				SFG_ERR("{0}", diagnostics);
				return false;
			}
		}

		return true;
	}

	bool plugin_type_schema_t::is_type_valid(const reflected_type_descriptor_t& type, size_t depth, string_t& diagnostics) const
	{
		if (depth > _types.size())
		{
			diagnostics = "Plugin reflection contains a recursive object layout.";
			return false;
		}

		const bool is_enum = type.flags.is_set(reflected_type_flag_enum);

		if (is_enum && (type.flags.is_set(PLUGIN_COMPONENT_FLAGS) || (type.size != sizeof(u8) && type.size != sizeof(u16) && type.size != sizeof(u32))))
		{
			diagnostics = "Plugin reflected enums must use an 8, 16 or 32 bit underlying type.";
			return false;
		}

		for (const reflected_field_descriptor_t& field : type.fields)
		{
			const auto first = std::find_if(type.fields.begin(), type.fields.end(), [&](const reflected_field_descriptor_t& other) { return other.field_identifier == field.field_identifier; });

			if (field.name == nullptr || field.name[0] == '\0' || field.field_identifier == 0 || &*first != &field)
			{
				diagnostics = "Plugin type " + string_t(type.name) + " contains invalid or duplicate fields.";
				return false;
			}

			if (is_enum)
				continue;

			if (!is_field_valid(type, field, depth, diagnostics))
				return false;
		}

		return true;
	}

	bool plugin_type_schema_t::is_field_valid(const reflected_type_descriptor_t& type, const reflected_field_descriptor_t& field, size_t depth, string_t& diagnostics) const
	{
		if (field.offset > type.size || field.size == 0 || field.size > type.size - field.offset)
		{
			diagnostics = "Plugin field is outside its type: " + string_t(type.name) + "." + field.name;
			return false;
		}

		size_t expected_size = field.size;

		switch (field.type)
		{
		case reflected_value_type_e::boolean:
		case reflected_value_type_e::u8:
		case reflected_value_type_e::i8:
			expected_size = 1;
			break;
		case reflected_value_type_e::u16:
		case reflected_value_type_e::i16:
			expected_size = 2;
			break;
		case reflected_value_type_e::f32:
		case reflected_value_type_e::u32:
		case reflected_value_type_e::i32:
			expected_size = 4;
			break;
		case reflected_value_type_e::u64:
		case reflected_value_type_e::i64:
			expected_size = 8;
			break;
		case reflected_value_type_e::char_array:
			break;
		case reflected_value_type_e::container: {
			const reflected_field_container_ops_t& ops		   = field.container_ops;
			const bool							   fixed_array = ops.storage == reflected_container_storage_e::fixed_array;
			const bool							   sized_array = ops.storage == reflected_container_storage_e::sized_array;

			if ((!fixed_array && !sized_array) || ops.capacity == 0 || ops.element_value_size == 0 || ops.element_offset > field.size || ops.capacity > (field.size - ops.element_offset) / ops.element_value_size || ops.get_element_ptr_fn == nullptr ||
				ops.get_element_size_fn == nullptr || (sized_array && (ops.count_offset > field.size || sizeof(size_t) > field.size - ops.count_offset || ops.add_element_ptr_fn == nullptr || ops.reset_fn == nullptr || ops.remove_index_fn == nullptr)))
			{
				diagnostics = "Plugin arrays require inline storage and valid layout metadata: " + string_t(type.name) + "." + field.name;
				return false;
			}

			if (sized_array && ops.count_offset < ops.element_offset + ops.capacity * ops.element_value_size && ops.element_offset < ops.count_offset + sizeof(size_t))
			{
				diagnostics = "Plugin array count overlaps its elements: " + string_t(type.name) + "." + field.name;
				return false;
			}

			if (ops.element_value_type == reflected_value_type_e::container || ops.element_value_type == reflected_value_type_e::bitmask)
			{
				diagnostics = "Plugin arrays require scalar or reflected object elements: " + string_t(type.name) + "." + field.name;
				return false;
			}

			const reflected_field_descriptor_t element = {
				.name		 = field.name,
				.sub_type_id = ops.element_sub_type_id,
				.size		 = ops.element_value_size,
				.type		 = ops.element_value_type,
			};

			if (!is_field_valid(type, element, depth, diagnostics))
				return false;

			break;
		}
		case reflected_value_type_e::bitmask:
			if ((field.size != 1 && field.size != 2 && field.size != 4 && field.size != 8) || type.bitmask_opts.get_option_count_fn == nullptr || type.bitmask_opts.get_option_fn == nullptr || type.bitmask_opts.build_title_fn == nullptr)
			{
				diagnostics = "Plugin bitmask field has invalid storage or callbacks: " + string_t(type.name) + "." + field.name;
				return false;
			}

			break;
		case reflected_value_type_e::object: {
			const reflected_type_descriptor_t* plugin	= find_type(field.sub_type_id);
			const reflected_type_t*			   existing = reflection_registry_t::get().find_type(field.sub_type_id);

			if (plugin != nullptr)
			{
				expected_size = plugin->size;

				if (!is_type_valid(*plugin, depth + 1, diagnostics))
					return false;
			}
			else if (existing != nullptr && existing->owner == reflection_owner_e::engine)
				expected_size = existing->size;
			else
			{
				diagnostics = "Plugin object field references an unavailable type: " + string_t(type.name) + "." + field.name;
				return false;
			}

			break;
		}
		default:
			diagnostics = "Plugin types support fixed-size value fields only: " + string_t(type.name) + "." + field.name;
			return false;
		}

		if (field.size != expected_size)
		{
			diagnostics = "Plugin field size does not match its reflected type: " + string_t(type.name) + "." + field.name;
			return false;
		}

		return true;
	}

	void plugin_type_schema_t::register_reflection_types() const
	{
		for (const reflected_type_descriptor_t& type : _types)
			reflection_registry_t::get().register_type(type);
	}

	bool plugin_type_schema_t::is_layout_equal(const plugin_type_schema_t& candidate, sid_t type_id) const
	{
		const reflected_type_descriptor_t& current_type	  = *find_type(type_id);
		const reflected_type_descriptor_t& candidate_type = *candidate.find_type(type_id);

		if (current_type.size != candidate_type.size || current_type.alignment != candidate_type.alignment || current_type.fields.size() != candidate_type.fields.size() ||
			(current_type.flags.value() & (PLUGIN_COMPONENT_FLAGS | reflected_type_flag_enum | reflected_type_flag_no_serialization)) !=
				(candidate_type.flags.value() & (PLUGIN_COMPONENT_FLAGS | reflected_type_flag_enum | reflected_type_flag_no_serialization)))
			return false;

		for (size_t index = 0; index < current_type.fields.size(); ++index)
		{
			const reflected_field_descriptor_t& field = current_type.fields[index];
			const reflected_field_descriptor_t& other = candidate_type.fields[index];

			if (field.field_identifier != other.field_identifier || field.offset != other.offset || field.size != other.size || field.type != other.type || field.sub_type_id != other.sub_type_id ||
				field.flags.is_set(reflected_field_flag_no_serialization) != other.flags.is_set(reflected_field_flag_no_serialization) || field.custom_serialization.to_stream_fn != other.custom_serialization.to_stream_fn ||
				field.custom_serialization.from_stream_fn != other.custom_serialization.from_stream_fn || field.custom_serialization.to_json_fn != other.custom_serialization.to_json_fn ||
				field.custom_serialization.from_json_fn != other.custom_serialization.from_json_fn)
				return false;

			if (field.type == reflected_value_type_e::container)
			{
				const reflected_field_container_ops_t& ops		 = field.container_ops;
				const reflected_field_container_ops_t& other_ops = other.container_ops;

				if (ops.storage != other_ops.storage || ops.capacity != other_ops.capacity || ops.element_offset != other_ops.element_offset || ops.count_offset != other_ops.count_offset || ops.element_value_type != other_ops.element_value_type ||
					ops.element_sub_type_id != other_ops.element_sub_type_id || ops.element_value_size != other_ops.element_value_size)
					return false;

				if (ops.element_value_type == reflected_value_type_e::object && find_type(ops.element_sub_type_id) != nullptr && !is_layout_equal(candidate, ops.element_sub_type_id))
					return false;
			}

			if (field.type == reflected_value_type_e::object && find_type(field.sub_type_id) != nullptr && !is_layout_equal(candidate, field.sub_type_id))
				return false;
		}

		return true;
	}

	plugin_type_schema_delta_t plugin_type_schema_t::compare(const plugin_type_schema_t& candidate) const
	{
		plugin_type_schema_delta_t delta = {};

		for (const reflected_type_descriptor_t& type : _types)
		{
			const reflected_type_descriptor_t* other = candidate.find_type(type.type_id);

			if (other == nullptr)
				delta.removed.push_back(type.type_id);
			else if (!is_layout_equal(candidate, type.type_id))
				delta.layout_changed.push_back(type.type_id);
		}

		for (const reflected_type_descriptor_t& type : candidate._types)
		{
			const reflected_type_descriptor_t* previous = find_type(type.type_id);

			if (previous == nullptr)
				delta.added.push_back(type.type_id);
		}

		return delta;
	}

	void plugin_type_schema_t::migrate_type(const plugin_type_schema_t& current, sid_t type_id, const void* source, void* destination) const
	{
		const reflected_type_descriptor_t& old_type = *current.find_type(type_id);
		const reflected_type_descriptor_t& new_type = *find_type(type_id);

		if (old_type.flags.is_set(reflected_type_flag_enum) || new_type.flags.is_set(reflected_type_flag_enum))
		{
			if (old_type.flags.is_set(reflected_type_flag_enum) && new_type.flags.is_set(reflected_type_flag_enum) && old_type.size == new_type.size)
				SFG_MEMCPY(destination, source, new_type.size);

			return;
		}

		for (const reflected_field_descriptor_t& field : new_type.fields)
		{
			const auto old = std::find_if(old_type.fields.begin(), old_type.fields.end(), [&](const reflected_field_descriptor_t& other) { return other.field_identifier == field.field_identifier; });

			if (old == old_type.fields.end() || old->type != field.type || old->sub_type_id != field.sub_type_id)
				continue;

			const u8* source_field		= static_cast<const u8*>(source) + old->offset;
			u8*		  destination_field = static_cast<u8*>(destination) + field.offset;

			if (field.type == reflected_value_type_e::container)
			{
				const reflected_field_container_ops_t& old_ops		 = old->container_ops;
				const reflected_field_container_ops_t& new_ops		 = field.container_ops;
				const bool							   plugin_object = new_ops.element_value_type == reflected_value_type_e::object && current.find_type(new_ops.element_sub_type_id) != nullptr && find_type(new_ops.element_sub_type_id) != nullptr;

				if (old_ops.element_value_type != new_ops.element_value_type || old_ops.element_sub_type_id != new_ops.element_sub_type_id || (!plugin_object && old_ops.element_value_size != new_ops.element_value_size))
					continue;

				size_t old_count = old_ops.capacity;

				if (old_ops.storage == reflected_container_storage_e::sized_array)
					SFG_MEMCPY(&old_count, source_field + old_ops.count_offset, sizeof(old_count));

				const size_t count		 = std::min(old_count, static_cast<size_t>(new_ops.capacity));
				const bool	 fixed_array = new_ops.storage == reflected_container_storage_e::fixed_array;

				if (!fixed_array)
					new_ops.reset_fn(destination_field);

				for (size_t index = 0; index < count; ++index)
				{
					const u8* source_element	  = source_field + old_ops.element_offset + index * old_ops.element_value_size;
					u8*		  destination_element = fixed_array ? new_ops.get_element_ptr_fn(destination_field, static_cast<u32>(index)) : new_ops.add_element_ptr_fn(destination_field);

					if (plugin_object)
						migrate_type(current, new_ops.element_sub_type_id, source_element, destination_element);
					else
						SFG_MEMCPY(destination_element, source_element, new_ops.element_value_size);
				}
			}
			else if (field.type == reflected_value_type_e::object && current.find_type(field.sub_type_id) != nullptr && find_type(field.sub_type_id) != nullptr)
				migrate_type(current, field.sub_type_id, source_field, destination_field);
			else if (old->size == field.size)
				SFG_MEMCPY(destination_field, source_field, field.size);
		}
	}
}
