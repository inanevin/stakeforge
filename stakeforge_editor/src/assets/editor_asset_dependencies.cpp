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

#include "assets/editor_asset_dependencies.hpp"
#include "assets/editor_asset.hpp"
#include "assets/editor_asset_io.hpp"

#include <sfg/io/assert.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/resources/animation_graph_def.hpp>
#include <sfg/runtime/resources/material_def.hpp>
#include <sfg/runtime/resources/ragdoll_def.hpp>
#include <sfg/runtime/resources/shader_data_definition.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	namespace
	{
		void append_dependency(sid_t sid, resource_type_e type, vector_t<editor_asset_dependency_t>& out_dependencies)
		{
			if (sid == NULL_SID)
				return;

			out_dependencies.push_back({
				.sid  = sid,
				.type = type,
			});
		}

		void fetch_reflected_dependencies(const reflected_type_t& type, const nlohmann::json& object_json, vector_t<editor_asset_dependency_t>& out_dependencies)
		{
			const reflection_registry_t& registry = reflection_registry_t::get();

			for (u32 field_index = type.fields.start; field_index < type.fields.end; ++field_index)
			{
				const reflected_field_t* field = registry.get_field(field_index);
				SFG_ASSERT(field != nullptr);

				if (field->value_type == reflected_value_type_e::u64)
				{
					const resource_type_e resource_type = resource_type_from_reflection_sub_type_id(field->sub_type_id);

					if (resource_type == resource_type_e::invalid)
						continue;

					append_dependency(object_json.value<sid_t>(field->name, NULL_SID), resource_type, out_dependencies);
					continue;
				}

				if (field->value_type == reflected_value_type_e::object)
				{
					const reflected_type_t* nested_type = registry.find_type(field->sub_type_id);
					SFG_ASSERT(nested_type != nullptr);

					const nlohmann::json nested_json = object_json.value<nlohmann::json>(field->name, nlohmann::json::object());

					fetch_reflected_dependencies(*nested_type, nested_json, out_dependencies);
					continue;
				}

				if (field->value_type != reflected_value_type_e::container)
					continue;

				const reflected_value_type_e element_value_type = field->container_ops.element_value_type;

				if (element_value_type != reflected_value_type_e::u64 && element_value_type != reflected_value_type_e::object)
					continue;

				const nlohmann::json elements_json = object_json.value<nlohmann::json>(field->name, nlohmann::json::array());

				if (!elements_json.is_array())
					continue;

				if (element_value_type == reflected_value_type_e::u64)
				{
					const resource_type_e resource_type = resource_type_from_reflection_sub_type_id(field->container_ops.element_sub_type_id);

					if (resource_type == resource_type_e::invalid)
						continue;

					for (const nlohmann::json& element_json : elements_json)
						append_dependency(element_json.get<sid_t>(), resource_type, out_dependencies);

					continue;
				}

				const reflected_type_t* element_type = registry.find_type(field->container_ops.element_sub_type_id);
				SFG_ASSERT(element_type != nullptr);

				for (const nlohmann::json& element_json : elements_json)
					fetch_reflected_dependencies(*element_type, element_json, out_dependencies);
			}
		}

		void fetch_component_dependencies(const nlohmann::json& components_json, vector_t<editor_asset_dependency_t>& out_dependencies)
		{
			if (!components_json.is_array())
				return;

			const reflection_registry_t& registry = reflection_registry_t::get();

			for (const nlohmann::json& component_json : components_json)
			{
				const sid_t				component_type_id = component_json.value<sid_t>("type", 0);
				const reflected_type_t* type			  = registry.find_type(component_type_id);

				if (type == nullptr)
					continue;

				const nlohmann::json component_data = component_json.value<nlohmann::json>("data", nlohmann::json::object());

				fetch_reflected_dependencies(*type, component_data, out_dependencies);
			}
		}

		void fetch_entity_group_dependencies(const nlohmann::json& entity_group_json, vector_t<editor_asset_dependency_t>& out_dependencies)
		{
			const nlohmann::json entities_json = entity_group_json.value<nlohmann::json>("local_entities", nlohmann::json::array());

			if (entities_json.is_array())
			{
				for (const nlohmann::json& entity_json : entities_json)
					append_dependency(entity_json.value<sid_t>("prefab", NULL_SID), resource_type_e::prefab, out_dependencies);
			}

			fetch_component_dependencies(entity_group_json.value<nlohmann::json>("components", nlohmann::json::array()), out_dependencies);
		}
	}

	bool editor_asset_dependencies_t::fetch_dependencies(const editor_asset_t& asset, vector_t<editor_asset_dependency_t>& out_dependencies)
	{
		const nlohmann::json embedded_source = editor_asset_io_t::get_embedded_source_json(asset);

		switch (asset.asset_type)
		{
		case editor_asset_type_e::material: {
			if (!embedded_source.is_object())
				return false;

			material_def_t material = {};
			embedded_source.get_to(material);

			out_dependencies.reserve(out_dependencies.size() + material.textures.size() + material.samplers.size() + 1);
			append_dependency(material.shader, resource_type_e::shader, out_dependencies);

			for (const material_texture_value_t& texture : material.textures)
				append_dependency(texture.texture, shader_texture_type_to_resource_type(texture.type), out_dependencies);

			for (const material_sampler_value_t& sampler : material.samplers)
				append_dependency(sampler.sampler, resource_type_e::texture_sampler, out_dependencies);

			break;
		}
		case editor_asset_type_e::animation_graph: {
			if (!embedded_source.is_object())
				return false;

			animation_graph_def_t animation_graph = {};

			if (!reflection_registry_t::get().type_from_json(type_id_t<animation_graph_def_t>::value, &animation_graph, nullptr, embedded_source))
				return false;

			append_dependency(animation_graph.target_skeleton, resource_type_e::skeleton, out_dependencies);

			for (const animation_graph_node_def_t& node : animation_graph.nodes)
			{
				for (const animation_graph_asm_state_def_t& state : node.asm_node.states)
				{
					for (const animation_graph_clip_def_t& clip : state.clips)
						append_dependency(clip.clip, resource_type_e::animation, out_dependencies);
				}
			}

			break;
		}
		case editor_asset_type_e::ragdoll: {
			if (!embedded_source.is_object())
				return false;

			ragdoll_def_t ragdoll = {};

			if (!reflection_registry_t::get().type_from_json(type_id_t<ragdoll_def_t>::value, &ragdoll, nullptr, embedded_source))
				return false;

			append_dependency(ragdoll.target_skeleton, resource_type_e::skeleton, out_dependencies);
			append_dependency(ragdoll.physical_material, resource_type_e::physical_material, out_dependencies);
			break;
		}
		case editor_asset_type_e::prefab:
			if (!embedded_source.is_object())
				return false;

			fetch_entity_group_dependencies(embedded_source, out_dependencies);
			break;
		case editor_asset_type_e::world: {
			if (!embedded_source.is_object())
				return false;

			const nlohmann::json root_entities_json = embedded_source.value<nlohmann::json>("root_entities", nlohmann::json::array());

			if (!root_entities_json.is_array())
				return false;

			for (const nlohmann::json& root_entity_json : root_entities_json)
				fetch_entity_group_dependencies(root_entity_json, out_dependencies);

			break;
		}
		default:
			break;
		}

		return true;
	}
}
