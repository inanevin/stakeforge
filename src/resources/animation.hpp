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

#include "animation_common.hpp"
#include "memory/chunk_handle.hpp"
#include "resources/common_resources.hpp"
#include "reflection/type_reflection.hpp"

namespace SFG
{
	struct animation_channel_v3_raw;
	struct animation_channel_q_raw;
	struct animation_raw;
	class chunk_allocator32;
	class world;

	struct animation_channel_v3
	{
		animation_interpolation interpolation = animation_interpolation::linear;
		chunk_handle32			keyframes;
		u16					keyframes_count = 0;
		i16					node_index		= -1;

		void	create_from_loader(const animation_channel_v3_raw& raw, chunk_allocator32& alloc);
		void	destroy(chunk_allocator32& alloc);
		vector3 sample(f32 time, chunk_allocator32& alloc) const;
	};

	struct animation_channel_q
	{
		animation_interpolation interpolation = animation_interpolation::linear;
		chunk_handle32			keyframes;
		u16					keyframes_count = 0;
		i16					node_index		= -1;

		void create_from_loader(const animation_channel_q_raw& raw, chunk_allocator32& alloc);
		void destroy(chunk_allocator32& alloc);
		quat sample(f32 time, chunk_allocator32& alloc) const;
	};

	class animation
	{
	public:
		static void reflect();
		// -----------------------------------------------------------------------------
		// resource
		// -----------------------------------------------------------------------------

		void create_from_loader(const animation_raw& raw, world& w, resource_handle handle);
		void destroy(world& w, resource_handle handle);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline chunk_handle32 get_position_channels() const
		{
			return _position_channels;
		}

		inline chunk_handle32 get_rotation_channels() const
		{
			return _rotation_channels;
		}

		inline chunk_handle32 get_scale_channels() const
		{
			return _scale_channels;
		}

		inline u16 get_position_channels_count() const
		{
			return _position_count;
		}

		inline u16 get_rotation_channels_count() const
		{
			return _rotation_count;
		}

		inline u16 get_scale_channels_count() const
		{
			return _scale_count;
		}

		inline f32 get_duration() const
		{
			return _duration;
		}

	private:
		f32 _duration = 0.0f;
#ifndef SFG_STRIP_DEBUG_NAMES
		chunk_handle32 _name;
#endif
		chunk_handle32 _position_channels;
		chunk_handle32 _scale_channels;
		chunk_handle32 _rotation_channels;
		u16		   _position_count = 0;
		u16		   _rotation_count = 0;
		u16		   _scale_count	   = 0;
	};

	REFLECT_TYPE(animation);
}
