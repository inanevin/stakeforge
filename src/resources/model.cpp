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

#include "model.hpp"
#include "memory/chunk_allocator.hpp"
#include "mesh.hpp"
#include "model_node.hpp"
#include "skin.hpp"
#include "animation.hpp"
#include "model_raw.hpp"
#include "texture.hpp"
#include "material.hpp"
#include "world/world.hpp"
#include "light_raw.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_events_gfx.hpp"
#include "reflection/reflection.hpp"

namespace SFG
{
	void model::reflect()
	{
		reflection::get().register_meta(type_id<model>::value, 0, "stkmodel");
	}

	model::~model()
	{
		SFG_ASSERT(!_flags.is_set(model::flags::created));
	}

	void model::create_from_loader(model_raw& raw, world& w, resource_handle handle)
	{
		SFG_ASSERT(!_flags.is_set(model::flags::created));
		_flags.set(model::flags::created);

		render_event_stream& stream = w.get_render_stream();
		resource_manager&	 rm		= w.get_resource_manager();
		chunk_allocator32&	 alloc	= rm.get_aux();

		if (!raw.name.empty())
			_name = alloc.allocate_text(raw.name);

		_total_aabb = raw.total_aabb;

		const u16 node_count		 = static_cast<u16>(raw.loaded_nodes.size());
		const u16 mesh_count		 = static_cast<u16>(raw.loaded_meshes.size());
		const u16 skins_count	 = static_cast<u16>(raw.loaded_skins.size());
		const u16 anims_count	 = static_cast<u16>(raw.loaded_animations.size());
		const u16 materials_count = static_cast<u16>(raw.loaded_materials.size());
		const u16 textures_count	 = static_cast<u16>(raw.loaded_textures.size());
		const u16 lights_count	 = static_cast<u16>(raw.loaded_lights.size());

		if (node_count != 0)
		{
			_nodes				  = alloc.allocate<model_node>(node_count);
			model_node* ptr_nodes = alloc.get<model_node>(_nodes);

			for (u16 i = 0; i < node_count; i++)
			{
				const model_node_raw& loaded_node = raw.loaded_nodes[i];
				model_node&			  node		  = ptr_nodes[i];
				node.create_from_loader(loaded_node, w, {});
			}
		}

		if (mesh_count != 0)
		{
			_created_meshes				= alloc.allocate<resource_handle>(mesh_count);
			resource_handle* meshes_ptr = alloc.get<resource_handle>(_created_meshes);

			for (u16 i = 0; i < mesh_count; i++)
			{
				const mesh_raw&		  loaded_mesh = raw.loaded_meshes[i];
				const resource_handle sub_handle  = rm.add_resource<mesh>(loaded_mesh.sid);
				meshes_ptr[i]					  = sub_handle;

				mesh& m = rm.get_resource<mesh>(sub_handle);
				m.create_from_loader(loaded_mesh, w, sub_handle);

				rm.store_relative_path(type_id<mesh>::value, sub_handle, loaded_mesh.name);
			}
		}

		if (skins_count != 0)
		{
			_created_skins			   = alloc.allocate<resource_handle>(skins_count);
			resource_handle* skins_ptr = alloc.get<resource_handle>(_created_skins);

			for (u16 i = 0; i < skins_count; i++)
			{
				const skin_raw&		  loaded_skin = raw.loaded_skins[i];
				const resource_handle sub_handle  = rm.add_resource<skin>(loaded_skin.sid);
				skins_ptr[i]					  = sub_handle;
				skin& created					  = rm.get_resource<skin>(sub_handle);
				created.create_from_loader(loaded_skin, w, sub_handle);
				rm.store_relative_path(type_id<skin>::value, sub_handle, loaded_skin.name);
			}
		}

		if (anims_count != 0)
		{
			_created_anims			   = alloc.allocate<resource_handle>(anims_count);
			resource_handle* anims_ptr = alloc.get<resource_handle>(_created_anims);

			for (u16 i = 0; i < anims_count; i++)
			{
				const animation_raw&  loaded_anim = raw.loaded_animations[i];
				const resource_handle sub_handle  = rm.add_resource<animation>(loaded_anim.sid);
				anims_ptr[i]					  = sub_handle;
				animation& created				  = rm.get_resource<animation>(sub_handle);
				created.create_from_loader(loaded_anim, w, sub_handle);
				rm.store_relative_path(type_id<animation>::value, sub_handle, loaded_anim.name);
			}
		}

		if (textures_count != 0)
		{
			_created_textures	 = alloc.allocate<resource_handle>(textures_count);
			resource_handle* ptr = alloc.get<resource_handle>(_created_textures);

			for (u16 i = 0; i < textures_count; i++)
			{
				const texture_raw&	  loaded	 = raw.loaded_textures[i];
				const resource_handle sub_handle = rm.add_resource<texture>(loaded.sid);
				ptr[i]							 = sub_handle;
				texture& created				 = rm.get_resource<texture>(sub_handle);
				created.create_from_loader(loaded, w, sub_handle);
				rm.store_relative_path(type_id<texture>::value, sub_handle, loaded.name);
			}
		}

		if (materials_count != 0)
		{
			_created_materials	 = alloc.allocate<resource_handle>(materials_count);
			resource_handle* ptr = alloc.get<resource_handle>(_created_materials);

			for (u16 i = 0; i < materials_count; i++)
			{
				material_raw&		  loaded	 = raw.loaded_materials[i];
				const resource_handle sub_handle = rm.add_resource<material>(loaded.sid);
				ptr[i]							 = sub_handle;

				material& created = rm.get_resource<material>(sub_handle);

				created.create_from_loader(loaded, w, sub_handle);
				rm.store_relative_path(type_id<material>::value, sub_handle, loaded.name);

				loaded.destroy();
			}
		}

		if (lights_count != 0)
		{
			_created_lights = alloc.allocate<light_raw>(lights_count);
			light_raw* ptr	= alloc.get<light_raw>(_created_lights);

			for (u16 i = 0; i < lights_count; i++)
			{
				const light_raw& loaded = raw.loaded_lights[i];
				ptr[i]					= raw.loaded_lights[i];
			}
		}

		_skins_count	 = skins_count;
		_anims_count	 = anims_count;
		_meshes_count	 = mesh_count;
		_nodes_count	 = node_count;
		_materials_count = materials_count;
		_textures_count	 = textures_count;
		_lights_count	 = lights_count;
	}

	void model::destroy(world& w, resource_handle handle)
	{
		if (!_flags.is_set(model::flags::created))
			return;
		_flags.remove(model::flags::created);

		render_event_stream& stream = w.get_render_stream();
		resource_manager&	 rm		= w.get_resource_manager();
		chunk_allocator32&	 alloc	= rm.get_aux();

		if (_name.size != 0)
			alloc.free(_name);
		_name = {};

		if (_nodes.size != 0)
		{
			model_node* ptr_nodes = alloc.get<model_node>(_nodes);

			for (u16 i = 0; i < _nodes_count; i++)
			{
				model_node& node = ptr_nodes[i];
				node.destroy(w, {});
			}

			alloc.free(_nodes);
		}

		if (_created_meshes.size != 0)
		{
			resource_handle* ptr = alloc.get<resource_handle>(_created_meshes);
			for (u16 i = 0; i < _meshes_count; i++)
			{
				const resource_handle sub_handle = ptr[i];
				if (!rm.is_valid<mesh>(sub_handle))
					continue;

				mesh& m = rm.get_resource<mesh>(sub_handle);
				m.destroy(w, sub_handle);
				rm.remove_resource<mesh>(sub_handle);
			}

			alloc.free(_created_meshes);
		}

		if (_created_skins.size != 0)
		{
			resource_handle* skins_ptr = alloc.get<resource_handle>(_created_skins);

			for (u16 i = 0; i < _skins_count; i++)
			{
				const resource_handle sub_handle = skins_ptr[i];
				if (!rm.is_valid<skin>(sub_handle))
					continue;
				skin& sk = rm.get_resource<skin>(sub_handle);
				sk.destroy(w, sub_handle);
				rm.remove_resource<skin>(sub_handle);
			}

			alloc.free(_created_skins);
		}

		if (_created_anims.size != 0)
		{
			resource_handle* anims_ptr = alloc.get<resource_handle>(_created_anims);

			for (u16 i = 0; i < _anims_count; i++)
			{
				const resource_handle sub_handle = anims_ptr[i];
				if (!rm.is_valid<animation>(sub_handle))
					continue;
				animation& anim = rm.get_resource<animation>(sub_handle);
				anim.destroy(w, sub_handle);
				rm.remove_resource<animation>(sub_handle);
			}

			alloc.free(_created_anims);
		}

		if (_created_textures.size != 0)
		{
			resource_handle* ptr = alloc.get<resource_handle>(_created_textures);

			for (u16 i = 0; i < _textures_count; i++)
			{
				const resource_handle sub_handle = ptr[i];
				if (!rm.is_valid<texture>(sub_handle))
					continue;
				texture& res = rm.get_resource<texture>(sub_handle);
				res.destroy(w, sub_handle);
				rm.remove_resource<texture>(sub_handle);
			}

			alloc.free(_created_textures);
		}

		if (_created_materials.size != 0)
		{
			resource_handle* ptr = alloc.get<resource_handle>(_created_materials);

			for (u16 i = 0; i < _materials_count; i++)
			{
				const resource_handle sub_handle = ptr[i];
				if (!rm.is_valid<material>(sub_handle))
					continue;
				material& res = rm.get_resource<material>(sub_handle);
				res.destroy(w, sub_handle);
				rm.remove_resource<material>(sub_handle);
			}

			alloc.free(_created_materials);
		}

		if (_created_lights.size != 0)
		{
			alloc.free(_created_lights);
		}

		_created_textures  = {};
		_created_materials = {};
		_created_meshes	   = {};
		_nodes			   = {};
		_created_skins	   = {};
		_created_anims	   = {};
		_created_lights	   = {};
	}

}
