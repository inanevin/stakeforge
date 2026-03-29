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

#include "component_manager.hpp"
#include "world.hpp"
#include "io/log.hpp"
#include "game/game_max_defines.hpp"

namespace SFG
{

	component_manager::component_manager(world& w) : _world(w)
	{
		_aux_memory.init(MAX_WORLD_COMPONENTS_AUX_MEMORY);
	}

	component_manager::~component_manager()
	{
		for (const comp_cache_storage& stg : _storages)
			delete stg.cache_ptr;

		_aux_memory.uninit();
	}

	void component_manager::init()
	{
	}

	void component_manager::uninit()
	{
		for (comp_cache_storage& stg : _storages)
			stg.cache_ptr->reset(_world);
		_aux_memory.reset();
	}

	world_handle component_manager::add_component(string_id type, world_handle entity)
	{
		entity_manager& em = _world.get_entity_manager();
		SFG_ASSERT(em.is_valid(entity));

		comp_cache_storage& stg	   = get_storage(type);
		const world_handle	handle = stg.cache_ptr->add(entity, _world);

		if (handle.is_null())
		{
			SFG_ERR("Failed adding component, type's pool is full! {0}", type);
			return {};
		}
		em.on_component_added(entity, handle, type);
		return handle;
	}

	void component_manager::remove_component(string_id type, world_handle entity, world_handle handle)
	{
		entity_manager& em = _world.get_entity_manager();
		SFG_ASSERT(em.is_valid(entity));

		comp_cache_storage& stg = get_storage(type);
		em.on_component_removed(entity, handle, type);
		stg.cache_ptr->remove(handle, _world);
	}

	bool component_manager::is_valid(string_id type, world_handle handle) const
	{
		const comp_cache_storage& stg = get_storage(type);
		return stg.cache_ptr->is_valid(handle);
	}

	void* component_manager::get_component(string_id type, world_handle handle) const
	{
		const comp_cache_storage& stg = get_storage(type);
		return stg.cache_ptr->get_ptr(handle);
	}

	const component_manager::comp_cache_storage& component_manager::get_storage(string_id type) const
	{
		auto it = std::find_if(_storages.begin(), _storages.end(), [type](const comp_cache_storage& stg) -> bool { return stg.type == type; });
		if (it == _storages.end())
		{
			SFG_ASSERT(false);
			throw std::runtime_error("Component storage not found, did you register it?");
		}

		return *it;
	}

	component_manager::comp_cache_storage& component_manager::get_storage(string_id type)
	{
		auto it = std::find_if(_storages.begin(), _storages.end(), [type](const comp_cache_storage& stg) -> bool { return stg.type == type; });
		if (it == _storages.end())
		{
			SFG_ASSERT(false);
			throw std::runtime_error("Component storage not found, did you register it?");
		}

		return *it;
	}

}
