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

#pragma once

#include "data/bitmask.hpp"
#include "world/components/common_comps.hpp"
#include "reflection/type_reflection.hpp"
#include "resources/common_resources.hpp"
#include "math/vector2ui16.hpp"
#include "memory/chunk_handle.hpp"

namespace vekt
{
	class builder;
	struct draw_buffer;
}

namespace SFG
{
	class world;

	class comp_canvas
	{
	public:
		enum flags
		{
			is_init = 1 << 0,
			is_3d	= 1 << 2,
		};

		static void reflect();

		// -----------------------------------------------------------------------------
		// trait
		// -----------------------------------------------------------------------------

		void on_add(world& w);
		void on_remove(world& w);

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void update_counts_and_init(world& w, u32 max_widget_count, u16 max_buffer_count);
		void set_is_3d(world& w, u8 is_3d);
		void draw(world& w, const vector2ui16& size);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline const bitmask<u8>& get_flags() const
		{
			return _flags;
		}

		inline const component_header& get_header() const
		{
			return _header;
		}

		inline vekt::builder* get_builder() const
		{
			return _builder;
		}

		// 0,0 for screen resolution
		inline void set_size(const vector2ui16& sz)
		{
			_size = sz;
		}

	private:
		void init_builder(world& w);
		void uninit_builder(world& w);
		void draw_vekt(const vekt::draw_buffer& buffer);

	private:
		template <typename T, int> friend class comp_cache;

	private:
		component_header _header		   = {};
		vekt::builder*	 _builder		   = nullptr;
		world*			 _world			   = nullptr;
		vector2ui16		 _size			   = vector2ui16::zero;
		u32				 _max_widget_count = 1024;
		u16				 _max_buffer_count = 50;
		u32				 _idx_counter	   = 0;
		u32				 _vtx_counter	   = 0;
		bitmask<u8>		 _flags			   = 0;
	};

	REFLECT_TYPE(comp_canvas);
}
