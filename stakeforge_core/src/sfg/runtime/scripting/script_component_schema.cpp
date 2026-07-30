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

#include "script_component_schema.hpp"

#include <sfg/io/log.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	namespace
	{
		reflected_value_type_e reflected_value_type_from_string(const string_t& value)
		{
			if (value == "f32")
				return reflected_value_type_e::f32;
			if (value == "u64")
				return reflected_value_type_e::u64;
			if (value == "i64")
				return reflected_value_type_e::i64;
			if (value == "u32")
				return reflected_value_type_e::u32;
			if (value == "i32")
				return reflected_value_type_e::i32;
			if (value == "u16")
				return reflected_value_type_e::u16;
			if (value == "i16")
				return reflected_value_type_e::i16;
			if (value == "u8")
				return reflected_value_type_e::u8;
			if (value == "i8")
				return reflected_value_type_e::i8;
			if (value == "boolean")
				return reflected_value_type_e::boolean;
			if (value == "object")
				return reflected_value_type_e::object;

			return reflected_value_type_e::invalid;
		}

		bool is_reflected_field_size_valid(reflected_value_type_e value_type, u32 size)
		{
			switch (value_type)
			{
			case reflected_value_type_e::f32:
			case reflected_value_type_e::u32:
			case reflected_value_type_e::i32:
				return size == 4;
			case reflected_value_type_e::u64:
			case reflected_value_type_e::i64:
				return size == 8;
			case reflected_value_type_e::u16:
			case reflected_value_type_e::i16:
				return size == 2;
			case reflected_value_type_e::u8:
			case reflected_value_type_e::i8:
			case reflected_value_type_e::boolean:
				return size == 1;
			case reflected_value_type_e::object:
				return true;
			default:
				return false;
			}
		}
	}

	const script_component_field_desc_t* script_component_desc_t::find_field(sid_t field_id) const
	{
		const auto it = std::find_if(fields.begin(), fields.end(), [field_id](const script_component_field_desc_t& field) { return field.field_id == field_id; });
		return it == fields.end() ? nullptr : &*it;
	}

	bool script_component_desc_t::is_layout_equal(const script_component_desc_t& other) const
	{
		if (size != other.size || alignment != other.alignment || fields.size() != other.fields.size())
			return false;

		for (size_t field_index = 0; field_index < fields.size(); ++field_index)
		{
			const script_component_field_desc_t& field		 = fields[field_index];
			const script_component_field_desc_t& other_field = other.fields[field_index];

			if (field.field_id != other_field.field_id || field.sub_type_id != other_field.sub_type_id || field.offset != other_field.offset || field.size != other_field.size || field.value_type != other_field.value_type)
				return false;
		}

		return true;
	}

	bool script_component_desc_t::is_reflection_equal(const script_component_desc_t& other) const
	{
		if (!is_layout_equal(other) || name != other.name || full_name != other.full_name)
			return false;

		for (size_t field_index = 0; field_index < fields.size(); ++field_index)
		{
			if (fields[field_index].name != other.fields[field_index].name || fields[field_index].flags != other.fields[field_index].flags)
				return false;
		}

		return true;
	}

	bool script_component_schema_delta_t::has_changes() const
	{
		return !added.empty() || !removed.empty() || !layout_changed.empty() || !reflection_changed.empty();
	}

	bool script_component_schema_t::parse(const char* schema_json)
	{
		const nlohmann::json document = nlohmann::json::parse(schema_json, nullptr, false);

		if (document.is_discarded())
		{
			SFG_ERR("managed component schema is not valid JSON.");
			return false;
		}

		const vector_t<nlohmann::json>	  component_jsons = document.value<vector_t<nlohmann::json>>("components", {});
		vector_t<script_component_desc_t> components	  = {};
		components.reserve(component_jsons.size());

		for (const nlohmann::json& component_json : component_jsons)
		{
			script_component_desc_t component = {};
			component.name					  = component_json.value<string_t>("name", "");
			component.full_name				  = component_json.value<string_t>("full_name", "");
			component.type_id				  = component_json.value<sid_t>("id", 0);
			component.size					  = component_json.value<u32>("size", 0);
			component.alignment				  = component_json.value<u32>("alignment", 0);

			if (component.name.empty() || component.full_name.empty() || component.type_id == 0 || component.type_id == NULL_SID || component.size == 0 || component.alignment == 0 || (component.alignment & (component.alignment - 1)) != 0)
			{
				SFG_ERR("managed component schema contains an invalid component descriptor.");
				return false;
			}

			const reflected_type_t* existing_type = reflection_registry_t::get().find_type(component.type_id);

			if (existing_type != nullptr && existing_type->owner != reflection_owner_e::game_scripts)
			{
				SFG_ERR("C# component {0} collides with engine reflection type {1}.", component.full_name, existing_type->name);
				return false;
			}

			if (std::find_if(components.begin(), components.end(), [&](const script_component_desc_t& other) { return other.type_id == component.type_id; }) != components.end())
			{
				SFG_ERR("managed component schema contains duplicate component id {0}.", component.type_id);
				return false;
			}

			const vector_t<nlohmann::json> field_jsons = component_json.value<vector_t<nlohmann::json>>("fields", {});
			component.fields.reserve(field_jsons.size());

			for (const nlohmann::json& field_json : field_jsons)
			{
				script_component_field_desc_t field = {};
				field.name							= field_json.value<string_t>("name", "");
				field.field_id						= field_json.value<sid_t>("id", 0);
				field.sub_type_id					= field_json.value<sid_t>("sub_type_id", 0);
				field.offset						= field_json.value<u32>("offset", 0);
				field.size							= field_json.value<u32>("size", 0);
				field.flags							= field_json.value<bool>("no_ui", false) ? reflected_field_flag_no_ui : 0;
				field.value_type					= reflected_value_type_from_string(field_json.value<string_t>("value_type", ""));

				if (field.name.empty() || field.field_id == 0 || field.size == 0 || field.value_type == reflected_value_type_e::invalid || !is_reflected_field_size_valid(field.value_type, field.size) ||
					static_cast<u64>(field.offset) + field.size > component.size)
				{
					SFG_ERR("managed component {0} contains an invalid field descriptor.", component.full_name);
					return false;
				}

				if (component.find_field(field.field_id) != nullptr)
				{
					SFG_ERR("managed component {0} contains duplicate field id {1}.", component.full_name, field.field_id);
					return false;
				}

				if (field.value_type == reflected_value_type_e::object)
				{
					const reflected_type_t* field_type = reflection_registry_t::get().find_type(field.sub_type_id);

					if (field_type == nullptr || field_type->size != field.size)
					{
						SFG_ERR("managed component field {0}.{1} has an invalid object type.", component.full_name, field.name);
						return false;
					}
				}

				component.fields.push_back(std::move(field));
			}

			components.push_back(std::move(component));
		}

		const vector_t<nlohmann::json>		 world_script_jsons = document.value<vector_t<nlohmann::json>>("world_scripts", {});
		vector_t<script_world_script_desc_t> world_scripts		= {};
		world_scripts.reserve(world_script_jsons.size());

		for (const nlohmann::json& world_script_json : world_script_jsons)
		{
			script_world_script_desc_t world_script = {
				.name	   = world_script_json.value<string_t>("name", ""),
				.full_name = world_script_json.value<string_t>("full_name", ""),
				.type_id   = world_script_json.value<sid_t>("id", 0),
			};

			if (world_script.name.empty() || world_script.full_name.empty() || world_script.type_id == 0 || world_script.type_id == NULL_SID)
			{
				SFG_ERR("managed component schema contains an invalid world script descriptor.");
				return false;
			}

			const auto duplicate = std::find_if(world_scripts.begin(), world_scripts.end(), [&](const script_world_script_desc_t& other) { return other.type_id == world_script.type_id; });

			if (duplicate != world_scripts.end())
			{
				SFG_ERR("managed component schema contains duplicate world script id {0}.", world_script.type_id);
				return false;
			}

			world_scripts.push_back(std::move(world_script));
		}

		_components	   = std::move(components);
		_world_scripts = std::move(world_scripts);

		return true;
	}

	void script_component_schema_t::register_reflection_types() const
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		for (const script_component_desc_t& component : _components)
		{
			vector_t<reflected_field_descriptor_t> fields = {};
			fields.reserve(component.fields.size());

			for (const script_component_field_desc_t& field : component.fields)
			{
				fields.push_back({
					.name			  = field.name.c_str(),
					.display_name	  = field.name.c_str(),
					.field_identifier = field.field_id,
					.sub_type_id	  = field.sub_type_id,
					.offset			  = field.offset,
					.size			  = field.size,
					.flags			  = field.flags,
					.type			  = field.value_type,
				});
			}

			registry.register_type({
				.name		  = component.full_name.c_str(),
				.display_name = component.name.c_str(),
				.category	  = "Scripting",
				.fields		  = std::move(fields),
				.type_id	  = component.type_id,
				.size		  = component.size,
				.alignment	  = component.alignment,
				.flags		  = reflected_type_flag_component | reflected_type_flag_script,
				.owner		  = reflection_owner_e::game_scripts,
			});
		}
	}

	script_component_schema_delta_t script_component_schema_t::compare(const script_component_schema_t& candidate) const
	{
		script_component_schema_delta_t delta = {};

		for (const script_component_desc_t& current_component : _components)
		{
			const script_component_desc_t* candidate_component = candidate.find_component(current_component.type_id);

			if (candidate_component == nullptr)
			{
				delta.removed.push_back(current_component.type_id);
				continue;
			}

			if (!current_component.is_layout_equal(*candidate_component))
			{
				delta.layout_changed.push_back(current_component.type_id);
				continue;
			}

			if (!current_component.is_reflection_equal(*candidate_component))
				delta.reflection_changed.push_back(current_component.type_id);
		}

		for (const script_component_desc_t& candidate_component : candidate._components)
		{
			if (find_component(candidate_component.type_id) == nullptr)
				delta.added.push_back(candidate_component.type_id);
		}

		return delta;
	}

	bool script_component_schema_t::is_equivalent(const script_component_schema_t& other) const
	{
		if (_components.size() != other._components.size() || _world_scripts.size() != other._world_scripts.size())
			return false;

		for (const script_component_desc_t& component : _components)
		{
			const script_component_desc_t* other_component = other.find_component(component.type_id);

			if (other_component == nullptr || !component.is_reflection_equal(*other_component))
				return false;
		}

		for (const script_world_script_desc_t& world_script : _world_scripts)
		{
			const script_world_script_desc_t* other_world_script = other.find_world_script(world_script.type_id);

			if (other_world_script == nullptr || world_script.name != other_world_script->name || world_script.full_name != other_world_script->full_name)
				return false;
		}

		return true;
	}

	const script_component_desc_t* script_component_schema_t::find_component(sid_t type_id) const
	{
		const auto it = std::find_if(_components.begin(), _components.end(), [type_id](const script_component_desc_t& component) { return component.type_id == type_id; });

		return it == _components.end() ? nullptr : &*it;
	}

	const script_world_script_desc_t* script_component_schema_t::find_world_script(sid_t type_id) const
	{
		const auto it = std::find_if(_world_scripts.begin(), _world_scripts.end(), [type_id](const script_world_script_desc_t& world_script) { return world_script.type_id == type_id; });

		return it == _world_scripts.end() ? nullptr : &*it;
	}

	size_t script_component_schema_t::get_field_count() const
	{
		size_t field_count = 0;

		for (const script_component_desc_t& component : _components)
			field_count += component.fields.size();

		return field_count;
	}
}
