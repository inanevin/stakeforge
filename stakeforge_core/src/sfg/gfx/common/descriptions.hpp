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

#include "format.hpp"
#include "gfx_constants.hpp"
#include "shader_description.hpp"
#include <sfg/math/vec2f.hpp>
#include <sfg/math/vec2u.hpp>
#include <sfg/math/vec2u16.hpp>
#include <sfg/data/bitmask.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/data/span.hpp>
#include <sfg/data/string.hpp>
#include <sfg/vendor/nhlohmann/json_fwd.hpp>

namespace sfg
{
	class ostream_t;
	class istream_t;

	struct viewport_t
	{
		vec2f_t	  pos	   = vec2f_t::zero;
		vec2u16_t size	   = vec2u16_t::zero;
		f32		  minDepth = 0.0f;
		f32		  maxDepth = 1.0f;
	};

	struct scissors_rect_t
	{
		vec2u16_t pos  = vec2u16_t::zero;
		vec2u16_t size = vec2u16_t::zero;
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

	enum binding_type_e : u8
	{
		constant,
		ubo,
		ssbo,
		uav,
		pointer,
		sampler_t,
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

	struct swapchain_desc_t
	{
		void*		  window_t	= nullptr;
		void*		  os_handle = nullptr;
		f32			  scaling	= 1.0f;
		format_e	  format	= format_e::undefined;
		vec2u16_t	  pos		= vec2u16_t::zero;
		vec2u16_t	  size		= vec2u16_t::zero;
		bitmask_t<u8> flags		= 0;
	};

	struct swapchain_recreate_desc_t
	{
		vec2u16_t			 size		 = vec2u16_t::zero;
		gfx_swapchain_handle swapchain_t = {};
		f32					 scaling	 = 1.0f;
		bitmask_t<u8>		 flags		 = 0;
	};

	struct resource_desc_t
	{
		static constexpr size_t MAX_DEBUG_NAME = 64;

		u32			   size						  = 0;
		u32			   structure_size			  = 0;
		u32			   structure_count			  = 0;
		bitmask_t<u16> flags					  = 0;
		char		   debug_name[MAX_DEBUG_NAME] = {"resource"};

		void set_name(const char* name);
	};

	enum class view_type : u8
	{
		sampled,
		render_target,
		depth_stencil,
		gpu_write,
	};

	struct view_desc_t
	{
		view_type type			 = view_type::sampled;
		u8		  base_arr_level = 0;
		u8		  level_count	 = 1;
		u8		  base_mip_level = 0;
		u8		  mip_count		 = 1;
		u8		  is_cubemap	 = 0;
		u8		  read_only		 = 0;
	};

	struct texture_desc_t
	{
		static constexpr size_t MAX_DEBUG_NAME = 64;
		static constexpr size_t MAX_VIEWS	   = 8;

		format_e	   texture_format			  = format_e::r8g8b8a8_srgb;
		format_e	   depth_stencil_format		  = format_e::d16_unorm;
		vec2u16_t	   size						  = vec2u16_t::zero;
		bitmask_t<u16> flags					  = 0;
		view_desc_t	   views[MAX_VIEWS]			  = {{}};
		u8			   view_count				  = 1;
		u8			   mip_levels				  = 1;
		u8			   array_length				  = 1;
		u8			   samples					  = 1;
		f32			   clear_values[4]			  = {0.0f, 0.0f, 0.0f, 1.0f};
		char		   debug_name[MAX_DEBUG_NAME] = {"texture"};

		void set_name(const char* name);
	};

	struct sampler_desc_t
	{
		static constexpr size_t MAX_DEBUG_NAME = 64;

		char		   debug_name[MAX_DEBUG_NAME] = {"sampler"};
		u32			   anisotropy				  = 0;
		f32			   min_lod					  = 0.0f;
		f32			   max_lod					  = 1.0f;
		f32			   lod_bias					  = 0.0f;
		bitmask_t<u16> flags					  = 0;
		address_mode   address_u				  = address_mode::clamp;
		address_mode   address_v				  = address_mode::clamp;
		address_mode   address_w				  = address_mode::clamp;
		compare_op	   compare					  = {};

		bool operator==(const sampler_desc_t& other) const;

		void set_name(const char* name);
		void serialize(ostream_t& stream) const;
		void deserialize(istream_t& stream);
	};

	struct layout_entry_t
	{
		binding_type_e type					  = binding_type_e::constant;
		u8			   count				  = 1;
		u8			   set					  = 0;
		u8			   binding_t			  = 0;
		sampler_desc_t immutable_sampler_desc = {};
	};

	struct binding_t
	{
		vector_t<layout_entry_t> entry_table;
		shader_stage_e			 visibility;
	};

	struct bind_layout_pointer_param_t
	{
		binding_type_e type		   = binding_type_e::ubo;
		u8			   set		   = 0;
		u8			   binding_t   = 0;
		u8			   count	   = 0;
		u8			   is_volatile = 0;
	};

	struct bind_group_pointer_t
	{
		gfx_resource_handle resource_t	  = {};
		gfx_texture_handle	texture_t	  = {};
		gfx_sampler_handle	sampler_t	  = {};
		u8					view		  = 0;
		u8					pointer_index = 0;
		binding_type_e		type		  = binding_type_e::ubo;
	};

	struct bind_group_binding_t
	{
		u8*			   constants  = nullptr;
		u8			   root_index = 0;
		u8			   count	  = 0;
		binding_type_e type		  = binding_type_e::constant;
	};

	struct binding_update_t
	{
		u32							  binding_index	 = 0;
		vector_t<binding_type_e>	  resource_types = {};
		vector_t<gfx_resource_handle> resources		 = {};
		vector_t<gfx_texture_handle>  textures		 = {};
		vector_t<gfx_sampler_handle>  samplers		 = {};
		vector_t<u32>				  resource_views = {};
	};

	struct bind_group_update_desc_t
	{
		vector_t<binding_update_t> updates;
	};

	struct queue_desc_t
	{
		command_type type			= command_type::graphics;
		char		 debug_name[16] = {"Debug Name"};
	};

	struct command_buffer_desc_t
	{
		command_type type			= command_type::graphics;
		char		 debug_name[16] = {"CmdBuffer"};
	};

	void to_json(nlohmann::json& j, const sampler_desc_t& s);
	void from_json(const nlohmann::json& j, sampler_desc_t& s);
}
