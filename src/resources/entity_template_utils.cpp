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

#include "entity_template_utils.hpp"
#include "entity_template_raw.hpp"
#include "math/vector2.hpp"
#include "reflection/reflection.hpp"
#include "world/entity_manager.hpp"
#include "world/component_manager.hpp"
#include "resources/resource_manager.hpp"
#include "resources/entity_template.hpp"
#include "common/type_id.hpp"
#include "math/vector2ui16.hpp"
#include "math/color.hpp"

#ifdef SFG_TOOLMODE
#include <vendor/nhlohmann/json.hpp>
using json = nlohmann::json;
#endif

namespace SFG
{
#ifdef SFG_TOOLMODE
	static inline void push_unique_path(vector<string>& list, const string& full)
	{
		string		 path_to_push = "";
		const size_t dot		  = full.find(".");
		const size_t slash		  = full.find_last_of("/");
		if (slash > dot)
			path_to_push = full.substr(0, slash);
		else
			path_to_push = full;

		auto it = std::find_if(list.begin(), list.end(), [&](const string& s) { return s.compare(path_to_push) == 0; });
		if (it == list.end())
			list.push_back(path_to_push);
	}

	void entity_template_utils::append_entity_components_as_json(
		json& out_components_array, world_handle entity, u32 entity_index, entity_manager& em, component_manager& cm, resource_manager& rm, const hash_map<u32, i32>& index_by_world, vector<string>& out_resource_paths)
	{
		const entity_comp_register& reg = em.get_component_register(entity);
		for (const entity_comp& c : reg.comps)
		{
			json comp_json		   = {};
			comp_json["comp_type"] = c.comp_type;
			comp_json["entity"]	   = entity_index;

			void*		ptr		  = cm.get_component(c.comp_type, c.comp_handle);
			meta&		comp_meta = reflection::get().resolve(c.comp_type);
			const auto& fields	  = comp_meta.get_fields();

			if (comp_meta.has_function("gather_resources"_hs))
			{
				vector<resource_handle_and_type> resources;
				comp_meta.invoke_function<void, void*, vector<resource_handle_and_type>&>("gather_resources"_hs, ptr, resources);
				for (const auto& ht : resources)
				{
					if (ht.handle.is_null())
						continue;
					const string path = rm.get_loaded_path_by_handle(ht.type_id, ht.handle);
					push_unique_path(out_resource_paths, path);
				}
			}

			for (field_base* f : fields)
			{
				const reflected_field_type ft = f->_type;
				if (ft == reflected_field_type::rf_float)
					comp_json[f->_title.c_str()] = f->value(ptr).cast<f32>();
				else if (ft == reflected_field_type::rf_int)
					comp_json[f->_title.c_str()] = f->value(ptr).cast<i32>();
				else if (ft == reflected_field_type::rf_uint)
					comp_json[f->_title.c_str()] = f->value(ptr).cast<u32>();
				else if (ft == reflected_field_type::rf_bool || ft == reflected_field_type::rf_uint8 || ft == reflected_field_type::rf_enum)
					comp_json[f->_title.c_str()] = f->value(ptr).cast<u8>();
				else if (ft == reflected_field_type::rf_vector2)
					comp_json[f->_title.c_str()] = f->value(ptr).cast<vector2>();
				else if (ft == reflected_field_type::rf_vector2ui16)
					comp_json[f->_title.c_str()] = f->value(ptr).cast<vector2ui16>();
				else if (ft == reflected_field_type::rf_vector3)
					comp_json[f->_title.c_str()] = f->value(ptr).cast<vector3>();
				else if (ft == reflected_field_type::rf_vector4)
					comp_json[f->_title.c_str()] = f->value(ptr).cast<vector4>();
				else if (ft == reflected_field_type::rf_color)
					comp_json[f->_title.c_str()] = f->value(ptr).cast<color>();
				else if (ft == reflected_field_type::rf_string)
					comp_json[f->_title.c_str()] = f->value(ptr).cast<string>();
				else if (ft == reflected_field_type::rf_resource)
				{
					if (f->_is_list)
					{
						const auto&	 v				 = f->value(ptr).cast_ref<vector<resource_handle>>();
						const u32 sz				 = static_cast<u32>(v.size());
						comp_json[f->_title.c_str()] = sz;
						for (u32 i = 0; i < sz; ++i)
						{
							const string key = string(f->_title.c_str()) + std::to_string(i);
							if (!v[i].is_null())
							{
								const string p		   = rm.get_loaded_path_by_handle(f->_sub_type_id, v[i]);
								comp_json[key.c_str()] = p;
							}
							else
								comp_json[key.c_str()] = "";
						}
					}
					else
					{
						const resource_handle h = f->value(ptr).cast<resource_handle>();
						if (!h.is_null())
							comp_json[f->_title.c_str()] = rm.get_loaded_path_by_handle(f->_sub_type_id, h);
						else
							comp_json[f->_title.c_str()] = "";
					}
				}
				else if (ft == reflected_field_type::rf_entity)
				{
					if (f->_is_list)
					{
						const auto&	 v				 = f->value(ptr).cast_ref<vector<world_handle>>();
						const u32 sz				 = static_cast<u32>(v.size());
						comp_json[f->_title.c_str()] = sz;
						for (u32 i = 0; i < sz; ++i)
						{
							const string key	   = string(f->_title.c_str()) + std::to_string(i);
							const i32	 idx	   = v[i].is_null() ? -1 : index_by_world.at(v[i].index);
							comp_json[key.c_str()] = idx;
						}
					}
					else
					{
						const world_handle h		 = f->value(ptr).cast<world_handle>();
						const i32		   idx		 = h.is_null() ? -1 : index_by_world.at(h.index);
						comp_json[f->_title.c_str()] = idx;
					}
				}
			}

			out_components_array.push_back(comp_json);
		}
	}

	void entity_template_utils::component_json_to_component_buffer(const json& c, ostream& out)
	{
		const string_id comp_type	= c.value<string_id>("comp_type", 0);
		const u32	e_index		= c.value<u32>("entity", 0);
		auto&			comp_meta	= reflection::get().resolve(comp_type);
		const auto&		fields		= comp_meta.get_fields();
		const u32	fields_size = static_cast<u32>(fields.size());

		out << comp_type;
		out << e_index;
		out << fields_size;

		for (field_base* f : fields)
		{
			const reflected_field_type ft = f->_type;
			out << f->_sid;
			out << ft;

			const char* key = f->_title.c_str();

			if (ft == reflected_field_type::rf_float)
			{
				out << c.value<f32>(key, 0.0f);
			}
			else if (ft == reflected_field_type::rf_int)
			{
				out << c.value<i32>(key, 0);
			}
			else if (ft == reflected_field_type::rf_uint)
			{
				out << c.value<u32>(key, 0);
			}
			else if (ft == reflected_field_type::rf_vector2)
			{
				out << c.value<vector2>(key, vector2::zero);
			}
			else if (ft == reflected_field_type::rf_vector2ui16)
			{
				out << c.value<vector2ui16>(key, vector2ui16());
			}
			else if (ft == reflected_field_type::rf_vector3)
			{
				out << c.value<vector3>(key, vector3::zero);
			}
			else if (ft == reflected_field_type::rf_vector4)
			{
				out << c.value<vector4>(key, vector4::zero);
			}
			else if (ft == reflected_field_type::rf_color)
			{
				out << c.value<color>(key, color::white);
			}
			else if (ft == reflected_field_type::rf_bool || ft == reflected_field_type::rf_uint8 || ft == reflected_field_type::rf_enum)
			{
				out << c.value<u8>(key, 0);
			}
			else if (ft == reflected_field_type::rf_string)
			{
				out << c.value<string>(key, "");
			}
			else if (ft == reflected_field_type::rf_resource)
			{
				out << f->_sub_type_id;
				if (f->_is_list)
				{
					const u32 count = c.value<u32>(key, 0);
					out << count;
					for (u32 i = 0; i < count; ++i)
					{
						const string k = string(key) + std::to_string(i);
						out << c.value<string>(k.c_str(), "");
					}
				}
				else
				{
					out << static_cast<u32>(1);
					out << c.value<string>(key, "");
				}
			}
			else if (ft == reflected_field_type::rf_entity)
			{
				if (f->_is_list)
				{
					const u32 count = c.value<u32>(key, 0);
					out << count;
					for (u32 i = 0; i < count; ++i)
					{
						const string k = string(key) + std::to_string(i);
						out << c.value<i32>(k.c_str(), -1);
					}
				}
				else
				{
					out << static_cast<u32>(1);
					out << c.value<i32>(key, -1);
				}
			}
		}
	}

	void entity_template_utils::entity_components_to_component_buffer(world_handle entity, u32 entity_index, entity_manager& em, component_manager& cm, resource_manager& rm, const hash_map<u32, i32>& index_by_world, ostream& out)
	{
		const entity_comp_register& reg = em.get_component_register(entity);
		for (const entity_comp& c : reg.comps)
		{
			out << c.comp_type;
			out << entity_index;

			void*		 ptr		 = cm.get_component(c.comp_type, c.comp_handle);
			meta&		 comp_meta	 = reflection::get().resolve(c.comp_type);
			const auto&	 fields		 = comp_meta.get_fields();
			const u32 fields_size = static_cast<u32>(fields.size());
			out << fields_size;

			for (field_base* f : fields)
			{
				const reflected_field_type ft = f->_type;
				out << f->_sid;
				out << ft;

				if (ft == reflected_field_type::rf_float)
					out << f->value(ptr).cast<f32>();
				else if (ft == reflected_field_type::rf_int)
					out << f->value(ptr).cast<i32>();
				else if (ft == reflected_field_type::rf_uint)
					out << f->value(ptr).cast<u32>();
				else if (ft == reflected_field_type::rf_vector2)
					out << f->value(ptr).cast<vector2>();
				else if (ft == reflected_field_type::rf_vector2ui16)
					out << f->value(ptr).cast<vector2ui16>();
				else if (ft == reflected_field_type::rf_vector3)
					out << f->value(ptr).cast<vector3>();
				else if (ft == reflected_field_type::rf_vector4)
					out << f->value(ptr).cast<vector4>();
				else if (ft == reflected_field_type::rf_color)
					out << f->value(ptr).cast<color>();
				else if (ft == reflected_field_type::rf_string)
					out << f->value(ptr).cast<string>();
				else if (ft == reflected_field_type::rf_uint8 || ft == reflected_field_type::rf_bool || ft == reflected_field_type::rf_enum)
					out << f->value(ptr).cast<u8>();
				else if (ft == reflected_field_type::rf_resource)
				{
					out << f->_sub_type_id;
					if (f->_is_list)
					{
						const auto&	 v	   = f->value(ptr).cast_ref<vector<resource_handle>>();
						const u32 count = static_cast<u32>(v.size());
						out << count;
						for (u32 i = 0; i < count; ++i)
						{
							const string p = v[i].is_null() ? string("") : rm.get_loaded_path_by_handle(f->_sub_type_id, v[i]);
							out << p;
						}
					}
					else
					{
						const resource_handle h = f->value(ptr).cast<resource_handle>();
						out << static_cast<u32>(1);
						out << (h.is_null() ? string("") : rm.get_loaded_path_by_handle(f->_sub_type_id, h));
					}
				}
				else if (ft == reflected_field_type::rf_entity)
				{
					if (f->_is_list)
					{
						const auto&	 v	   = f->value(ptr).cast_ref<vector<world_handle>>();
						const u32 count = static_cast<u32>(v.size());
						out << count;
						for (u32 i = 0; i < count; ++i)
						{
							const i32 idx = v[i].is_null() ? -1 : index_by_world.at(v[i].index);
							out << idx;
						}
					}
					else
					{
						const world_handle h   = f->value(ptr).cast<world_handle>();
						const i32		   idx = h.is_null() ? -1 : index_by_world.at(h.index);
						out << static_cast<u32>(1);
						out << idx;
					}
				}
			}
		}
	}

	entity_template_entity_raw entity_template_utils::entity_to_entity_template_entity_raw(world_handle entity, entity_manager& em, resource_manager& rm, const hash_map<u32, i32>& index_by_world)
	{
		entity_template_entity_raw er = {};
		const entity_meta&		   m  = em.get_entity_meta(entity);
		er.name						  = m.name;
		er.tag						  = m.tag;
		er.position					  = em.get_entity_position(entity);
		er.scale					  = em.get_entity_scale(entity);
		er.rotation					  = em.get_entity_rotation(entity);
		er.parent					  = -1;
		er.first_child				  = -1;
		er.next_sibling				  = -1;
		er.visible					  = !em.get_entity_flags(entity).is_set(entity_flags::entity_flags_invisible);

		const resource_handle tmp = em.get_entity_template_ref(entity);
		if (!tmp.is_null())
			er.template_reference = rm.get_loaded_path_by_handle(type_id<entity_template>::value, tmp);

		const entity_family& fam = em.get_entity_family(entity);
		if (!fam.parent.is_null())
		{
			auto it = index_by_world.find(fam.parent.index);
			if (it != index_by_world.end())
				er.parent = it->second;
		}
		if (!fam.first_child.is_null())
		{
			auto it = index_by_world.find(fam.first_child.index);
			if (it != index_by_world.end())
				er.first_child = it->second;
		}
		if (!fam.next_sibling.is_null())
		{
			auto it = index_by_world.find(fam.next_sibling.index);
			if (it != index_by_world.end())
				er.next_sibling = it->second;
		}

		return er;
	}
#endif // SFG_TOOLMODE

	void entity_template_utils::fill_components_from_buffer(istream& in, const static_vector<world_handle, 1024>& created, component_manager& cm, resource_manager& rm, world& w)
	{
		while (!in.is_eof())
		{
			string_id comp_type = 0;
			u32	  e_index	= 0;
			u32	  fields_sz = 0;
			in >> comp_type;
			if (in.is_eof())
				break;
			in >> e_index;
			in >> fields_sz;

			const world_handle entity_handle = created[e_index];
			const world_handle comp_handle	 = cm.add_component(comp_type, entity_handle);
			void*			   comp_ptr		 = cm.get_component(comp_type, comp_handle);

			meta&		comp_meta  = reflection::get().resolve(comp_type);
			const auto& ref_fields = comp_meta.get_fields();

			for (u32 j = 0; j < fields_sz; ++j)
			{
				string_id			 title_sid = 0;
				reflected_field_type ft		   = reflected_field_type::rf_float;
				in >> title_sid;
				in >> ft;

				field_base* target_field = nullptr;
				auto		it			 = std::find_if(ref_fields.begin(), ref_fields.end(), [title_sid](field_base* fb) { return fb->_sid == title_sid; });
				if (it != ref_fields.end())
					target_field = *it;

				if (ft == reflected_field_type::rf_float)
				{
					f32 val = 0.0f;
					in >> val;
					if (target_field)
						target_field->value(comp_ptr).cast_ref<f32>() = val;
				}
				else if (ft == reflected_field_type::rf_int)
				{
					i32 val = 0;
					in >> val;
					if (target_field)
						target_field->value(comp_ptr).cast_ref<i32>() = val;
				}
				else if (ft == reflected_field_type::rf_uint)
				{
					u32 val = 0;
					in >> val;
					if (target_field)
						target_field->value(comp_ptr).cast_ref<u32>() = val;
				}
				else if (ft == reflected_field_type::rf_vector2)
				{
					vector2 val = vector2::zero;
					in >> val;
					if (target_field)
						target_field->value(comp_ptr).cast_ref<vector2>() = val;
				}
				else if (ft == reflected_field_type::rf_vector2ui16)
				{
					vector2ui16 val;
					in >> val;
					if (target_field)
						target_field->value(comp_ptr).cast_ref<vector2ui16>() = val;
				}
				else if (ft == reflected_field_type::rf_vector3)
				{
					vector3 val = vector3::zero;
					in >> val;
					if (target_field)
						target_field->value(comp_ptr).cast_ref<vector3>() = val;
				}
				else if (ft == reflected_field_type::rf_vector4)
				{
					vector4 val = vector4::zero;
					in >> val;
					if (target_field)
						target_field->value(comp_ptr).cast_ref<vector4>() = val;
				}
				else if (ft == reflected_field_type::rf_color)
				{
					color val = color::white;
					in >> val;
					if (target_field)
						target_field->value(comp_ptr).cast_ref<color>() = val;
				}
				else if (ft == reflected_field_type::rf_string)
				{
					string val = "";
					in >> val;
					if (target_field)
						target_field->value(comp_ptr).cast_ref<string>() = val;
				}
				else if (ft == reflected_field_type::rf_uint8 || ft == reflected_field_type::rf_bool || ft == reflected_field_type::rf_enum)
				{
					u8 val = 0;
					in >> val;
					if (target_field)
						target_field->value(comp_ptr).cast_ref<u8>() = val;
				}
				else if (ft == reflected_field_type::rf_resource)
				{
					string_id sub_type = 0;
					u32	  count	   = 0;
					in >> sub_type;
					in >> count;
					for (u32 i = 0; i < count; ++i)
					{
						string val = "";
						in >> val;
						if (!target_field)
							continue;

						if (target_field->_is_list)
						{
							auto& v = target_field->value(comp_ptr).cast_ref<vector<resource_handle>>();
							v.push_back(val.empty() ? resource_handle() : rm.get_resource_handle_by_hash(sub_type, TO_SID(val)));
						}
						else
						{
							target_field->value(comp_ptr).cast_ref<resource_handle>() = val.empty() ? resource_handle() : rm.get_resource_handle_by_hash_if_exists(sub_type, TO_SID(val));
						}
					}
				}
				else if (ft == reflected_field_type::rf_entity)
				{
					u32 count = 0;
					in >> count;
					for (u32 i = 0; i < count; ++i)
					{
						i32 val = -1;
						in >> val;
						if (!target_field)
							continue;

						if (target_field->_is_list)
						{
							auto& v = target_field->value(comp_ptr).cast_ref<vector<world_handle>>();
							v.push_back(val == -1 ? world_handle() : created[val]);
						}
						else
						{
							target_field->value(comp_ptr).cast_ref<world_handle>() = (val == -1 ? world_handle() : created[val]);
						}
					}
				}
			}

			if (comp_meta.has_function("on_reflect_load"_hs))
				comp_meta.invoke_function<void, void*, world&>("on_reflect_load"_hs, comp_ptr, w);
		}
	}
}
