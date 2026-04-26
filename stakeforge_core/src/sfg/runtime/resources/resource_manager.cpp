// Copyright (c) 2025 Inan Evin

#include "resource_manager.hpp"
#include "animation.hpp"
#include "animation_state_machine.hpp"
#include "audio.hpp"
#include "font.hpp"
#include "io/assert.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "particle_properties.hpp"
#include "physical_material.hpp"
#include "prefab.hpp"
#include "shader.hpp"
#include "skeleton.hpp"
#include "texture.hpp"
#include "texture_sampler.hpp"

namespace sfg
{
	void resource_manager_t::init(size_t resource_memory_size)
	{
		SFG_ASSERT(resource_memory_size != 0);
		_memory.init(resource_memory_size);

		register_audio_resource(*this);
		register_font_resource(*this);
		register_mesh_resource(*this);
		register_skeleton_resource(*this);
		register_animation_resource(*this);
		register_particle_properties_resource(*this);
		register_material_resource(*this);
		register_shader_resource(*this);
		register_texture_resource(*this);
		register_texture_sampler_resource(*this);
		register_physical_material_resource(*this);
		register_prefab_resource(*this);
		register_animation_state_machine_resource(*this);
	}

	void resource_manager_t::uninit()
	{
		_entries.clear();
		_type_descs.clear();
		_memory.uninit();
	}

	resource_entry_t* resource_manager_t::find_entry(u64 hash)
	{
		auto it = _entries.find(hash);
		if (it == _entries.end())
			return nullptr;

		return &it->second;
	}

	const resource_entry_t* resource_manager_t::find_entry(u64 hash) const
	{
		auto it = _entries.find(hash);
		if (it == _entries.end())
			return nullptr;

		return &it->second;
	}

	void resource_manager_t::register_type_desc(const resource_type_desc_t& desc)
	{
		const u8 type = static_cast<u8>(desc.type);
		SFG_ASSERT(desc.type != resource_type_t::invalid);
		SFG_ASSERT(type < resource_type_max);
		SFG_ASSERT(_type_descs.find(type) == _type_descs.end());
		_type_descs.emplace(type, desc);
	}

	resource_type_desc_t* resource_manager_t::find_type_desc(resource_type_t type)
	{
		auto it = _type_descs.find(static_cast<u8>(type));
		if (it == _type_descs.end())
			return nullptr;

		return &it->second;
	}

	const resource_type_desc_t* resource_manager_t::find_type_desc(resource_type_t type) const
	{
		auto it = _type_descs.find(static_cast<u8>(type));
		if (it == _type_descs.end())
			return nullptr;

		return &it->second;
	}
}
