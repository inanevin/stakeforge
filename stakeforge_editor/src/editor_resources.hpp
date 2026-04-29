// Copyright (c) 2025 Inan Evin
#pragma once

#include "common/string_id.hpp"
#include "data/hash_map.hpp"
#include "gfx/common/gfx_constants.hpp"
#include "io/assert.hpp"
#include "memory/bump_allocator.hpp"

namespace sfg
{
	struct shader_desc_t;

	enum class editor_resource_type_e : u8
	{
		texture,
		shader,
	};

	struct editor_texture_t
	{
		gfx_texture_handle handle = {};
	};

	struct editor_shader_t
	{
		gfx_shader_handle handle = {};
	};

	class editor_resources_t
	{
	public:
		struct entry_t
		{
			void*				   ptr	= nullptr;
			editor_resource_type_e type = editor_resource_type_e::texture;
		};

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		bool init();
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		editor_shader_t*  load_shader(const char* path, const shader_desc_t& desc);
		editor_texture_t* load_texture(const char* path);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		template <typename T> const T& get_resource(string_id sid, editor_resource_type_e type) const
		{
			const auto it = _resources.find(sid);
			SFG_ASSERT(it != _resources.end());
			SFG_ASSERT(it->second.type == type);
			return *static_cast<const T*>(it->second.ptr);
		}

	private:
		bump_allocator_t			   _allocator = {};
		hash_map_t<string_id, entry_t> _resources = {};
	};
}
