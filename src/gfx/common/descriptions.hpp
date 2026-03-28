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

#include "math/vector2.hpp"
#include "math/vector2ui.hpp"
#include "math/vector2ui16.hpp"
#include "data/bitmask.hpp"
#include "data/vector.hpp"
#include "data/span.hpp"
#include "data/string.hpp"
#include "format.hpp"
#include "gfx/common/gfx_constants.hpp"
#include "shader_description.hpp"

#ifdef SFG_JSON_SERIALIZE
#include "vendor/nhlohmann/json_fwd.hpp"
#endif

namespace SFG
{
	class ostream;
	class istream;

	struct viewport
	{
		vec2f		pos		 = vec2f::zero;
		vec2u16 size	 = vec2u16::zero;
		f32			minDepth = 0.0f;
		f32			maxDepth = 1.0f;
	};

	struct scissors_rect
	{
		vec2u16 pos	 = vec2u16::zero;
		vec2u16 size = vec2u16::zero;
	};

	enum class command_type : u8
	{
		graphics,
		transfer,
		compute,
	};

	enum resource_flags
	{
		rf_vertex_buffer   = 1 << 0,
		rf_index_buffer	   = 1 << 1,
		rf_constant_buffer = 1 << 2,
		rf_storage_buffer  = 1 << 3,
		rf_indirect_buffer = 1 << 4,
		rf_gpu_only		   = 1 << 5,
		rf_cpu_visible	   = 1 << 6,
		rf_gpu_write	   = 1 << 7,
		rf_readback		   = 1 << 8,
	};

	enum swapchain_flags
	{
		sf_is_full_screen		= 1 << 0,
		sf_vsync_every_v_blank	= 1 << 1,
		sf_vsync_every_2v_blank = 1 << 2,
		sf_allow_tearing		= 1 << 3,
	};

	enum binding_type : u8
	{
		constant,
		ubo,
		ssbo,
		uav,
		pointer,
		sampler,
		texture_binding,
	};

	enum binding_flags
	{
		bf_ubo		 = 1 << 0,
		bf_ssbo		 = 1 << 1,
		bf_uav		 = 1 << 2,
		bf_constant	 = 1 << 3,
		bf_table	 = 1 << 4,
		bf_unbounded = 1 << 5,
		bf_sampler	 = 1 << 6,
		bf_texture	 = 1 << 7,
	};

	enum texture_flags
	{
		tf_render_target			= 1 << 0,
		tf_depth_texture			= 1 << 1,
		tf_stencil_texture			= 1 << 2,
		tf_linear_tiling			= 1 << 3,
		tf_sampled					= 1 << 4,
		tf_sampled_outside_fragment = 1 << 5,
		tf_transfer_source			= 1 << 6,
		tf_transfer_dest			= 1 << 7,
		tf_cubemap					= 1 << 8,
		tf_readback					= 1 << 9,
		tf_is_2d					= 1 << 10,
		tf_is_3d					= 1 << 11,
		tf_is_1d					= 1 << 12,
		tf_shared					= 1 << 13,
		tf_typeless					= 1 << 14,
		tf_gpu_write				= 1 << 15,
	};

	enum sampler_flags
	{
		saf_min_anisotropic	   = 1 << 0,
		saf_min_nearest		   = 1 << 1,
		saf_min_linear		   = 1 << 2,
		saf_mag_anisotropic	   = 1 << 3,
		saf_mag_nearest		   = 1 << 4,
		saf_mag_linear		   = 1 << 5,
		saf_mip_nearest		   = 1 << 6,
		saf_mip_linear		   = 1 << 7,
		saf_border_transparent = 1 << 8,
		saf_border_white	   = 1 << 9,
		saf_compare			   = 1 << 10,
	};

	enum class address_mode : u8
	{
		repeat,
		border,
		clamp,
		mirrored_repeat,
		mirrored_clamp,
	};

	struct swapchain_desc
	{
		void*		window	  = nullptr;
		void*		os_handle = nullptr;
		f32			scaling	  = 1.0f;
		format		format	  = format::undefined;
		vec2u16 pos		  = vec2u16::zero;
		vec2u16 size	  = vec2u16::zero;
		bitmask<u8> flags	  = 0;
	};

	struct swapchain_recreate_desc
	{
		vec2u16 size	  = vec2u16::zero;
		gfx_id		swapchain = 0;
		f32			scaling	  = 1.0f;
		bitmask<u8> flags	  = 0;
	};

	struct resource_desc
	{
		u32			 size			 = 0;
		u32			 structure_size	 = 0;
		u32			 structure_count = 0;
		bitmask<u16> flags			 = 0;
		const char*	 debug_name		 = "resource";
	};

	enum class view_type : u8
	{
		sampled,
		render_target,
		depth_stencil,
		gpu_write,
	};
	struct view_desc
	{
		view_type type			 = view_type::sampled;
		u8		  base_arr_level = 0;
		u8		  level_count	 = 1;
		u8		  base_mip_level = 0;
		u8		  mip_count		 = 1;
		u8		  is_cubemap	 = 0;
		u8		  read_only		 = 0;
	};

	struct texture_desc
	{
		format			  texture_format	   = format::r8g8b8a8_srgb;
		format			  depth_stencil_format = format::d16_unorm;
		vec2u16		  size				   = vec2u16::zero;
		bitmask<u16>	  flags				   = 0;
		vector<view_desc> views				   = {
			   {},
		   };
		u8			mip_levels		= 1;
		u8			array_length	= 1;
		u8			samples			= 1;
		f32			clear_values[4] = {0.0f, 0.0f, 0.0f, 1.0f};
		const char* debug_name		= "texture";
	};

	struct sampler_desc
	{
		string		 debug_name = "sampler";
		u32			 anisotropy = 0;
		f32			 min_lod	= 0.0f;
		f32			 max_lod	= 1.0f;
		f32			 lod_bias	= 0.0f;
		bitmask<u16> flags		= 0;
		address_mode address_u	= address_mode::clamp;
		address_mode address_v	= address_mode::clamp;
		address_mode address_w	= address_mode::clamp;
		compare_op	 compare	= {};

		bool operator==(const sampler_desc& other) const;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct layout_entry
	{
		binding_type type					= binding_type::constant;
		u8			 count					= 1;
		u8			 set					= 0;
		u8			 binding				= 0;
		sampler_desc immutable_sampler_desc = {};
	};

	struct binding
	{
		vector<layout_entry> entry_table;
		shader_stage		 visibility;
	};

	struct bind_layout_pointer_param
	{
		binding_type type		 = binding_type::ubo;
		u8			 set		 = 0;
		u8			 binding	 = 0;
		u8			 count		 = 0;
		u8			 is_volatile = 0;
	};

	struct bind_group_pointer
	{
		gfx_id		 resource	   = 0;
		u8			 view		   = 0;
		u8			 pointer_index = 0;
		binding_type type		   = binding_type::ubo;
	};

	struct bind_group_binding
	{
		u8*			 constants	= nullptr;
		u8			 root_index = 0;
		u8			 count		= 0;
		binding_type type		= binding_type::constant;
	};

	struct binding_update
	{
		u32					 binding_index	= 0;
		vector<binding_type> resource_types = {};
		vector<gfx_id>		 resources		= {};
		vector<u32>			 resource_views = {};
	};

	struct bind_group_update_desc
	{
		vector<binding_update> updates;
	};

	struct queue_desc
	{
		command_type type			= command_type::graphics;
		char		 debug_name[16] = {"Debug Name"};
	};

	struct command_buffer_desc
	{
		command_type type			= command_type::graphics;
		char		 debug_name[16] = {"CmdBuffer"};
	};

#ifdef SFG_JSON_SERIALIZE
	void to_json(nlohmann::json& j, const sampler_desc& s);
	void from_json(const nlohmann::json& j, sampler_desc& s);
#endif
}
