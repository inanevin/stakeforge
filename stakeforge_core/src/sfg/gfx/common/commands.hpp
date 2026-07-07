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

#include "descriptions.hpp"
#include "barrier_description.hpp"
#include <sfg/common/size_definitions.hpp>
#include <sfg/math/vec4f.hpp>
#include <sfg/data/bitmask.hpp>

namespace sfg
{
	struct texture_buffer_t;

	enum render_pass_op_flags
	{
		rpa_load_op_clear	   = 1 << 0,
		rpa_load_op_load	   = 1 << 1,
		rpa_load_op_dont_care  = 1 << 2,
		rpa_store_op_clear	   = 1 << 3,
		rpa_store_op_load	   = 1 << 4,
		rpa_store_op_dont_care = 1 << 5,
	};

	enum render_pass_att_flags
	{
		rpt_is_swapchain = 1 << 0,
		rpt_use_depth	 = 1 << 1,
		rpt_use_stencil	 = 1 << 2,
	};

	enum resource_state_flags
	{
		rsf_transfer_read  = 1 << 0,
		rsf_transfer_write = 1 << 1,
	};

	enum texture_state_flags
	{
		tsf_color_att	  = 1 << 0,
		tsf_depth_att	  = 1 << 1,
		tsf_shader_read	  = 1 << 2,
		tsf_depth_read	  = 1 << 3,
		tsf_stencil_read  = 1 << 4,
		tsf_present		  = 1 << 5,
		tsf_transfer_src  = 1 << 6,
		tsf_transfer_dest = 1 << 7,
	};

	enum render_pass_resolve_flags
	{
		rpa_resolve_mode_min = 1 << 0,
		rpa_resolve_mode_avg = 1 << 1,
	};

	struct render_pass_color_attachment_t
	{
		vec4f_t		 clear_color = vec4f_t(0, 0, 0, 1);
		gfx_handle_t texture	 = {};
		load_op		 load_op	 = load_op::clear;
		store_op	 store_op	 = store_op::store;
		u8			 view_index	 = 0;
	};

	struct render_pass_swapchain_attachment_t
	{
		vec4f_t		 clear_color = vec4f_t(0, 0, 0, 1);
		gfx_handle_t swapchain	 = {};
		load_op		 load_op	 = load_op::clear;
		store_op	 store_op	 = store_op::store;
		u8			 view_index	 = 0;
	};

	struct render_pass_depth_stencil_attachment_t
	{
		gfx_handle_t texture		  = {};
		u8			 clear_stencil	  = 0;
		f32			 clear_depth	  = 1.0f;
		load_op		 depth_load_op	  = load_op::clear;
		load_op		 stencil_load_op  = load_op::none;
		store_op	 depth_store_op	  = store_op::dont_care;
		store_op	 stencil_store_op = store_op::none;
		u8			 view_index		  = 0;
	};

	struct command_begin_render_pass_t
	{
		static constexpr u8 TID = 0;

		const render_pass_color_attachment_t* color_attachments		 = {};
		u8									  color_attachment_count = 0;
	};

	struct command_end_render_pass_t
	{
		static constexpr u8 TID = 1;
	};

	struct command_set_viewport_t
	{
		static constexpr u8 TID = 2;

		f32 x		  = 0.0f;
		f32 y		  = 0.0f;
		f32 min_depth = 0.0f;
		f32 max_depth = 0.0f;
		u16 width	  = 0;
		u16 height	  = 0;
	};

	struct command_set_scissors_t
	{
		static constexpr u8 TID = 3;

		u16 x	   = 0;
		u16 y	   = 0;
		u16 width  = 0;
		u16 height = 0;
	};

	struct command_bind_pipeline_t
	{
		static constexpr u8 TID = 4;

		gfx_handle_t pipeline = {};
	};

	struct command_draw_instanced_t
	{
		static constexpr u8 TID = 5;

		u32 vertex_count_per_instance = 0;
		u32 instance_count			  = 0;
		u32 start_vertex_location	  = 0;
		u32 start_instance_location	  = 0;
	};

	struct command_draw_indexed_instanced_t
	{
		static constexpr u8 TID = 6;

		u32 index_count_per_instance = 0;
		u32 instance_count			 = 0;
		u32 start_index_location	 = 0;
		u32 base_vertex_location	 = 0;
		u32 start_instance_location	 = 0;
	};

	struct command_draw_indexed_indirect_t
	{
		static constexpr u8 TID = 7;

		gfx_handle_t indirect_buffer		= {};
		u32			 indirect_buffer_offset = 0;
		u16			 count					= 0;
		gfx_handle_t indirect_signature_t	= {};
	};

	struct command_draw_indirect_t
	{
		static constexpr u8 TID = 8;

		gfx_handle_t indirect_buffer		= {};
		u32			 indirect_buffer_offset = 0;
		u16			 count					= 0;
		gfx_handle_t indirect_signature_t	= {};
	};

	struct command_copy_resource_t
	{
		static constexpr u8 TID = 9;

		gfx_handle_t source		 = {};
		gfx_handle_t destination = {};
	};

	struct command_copy_texture_to_buffer_t
	{
		static constexpr u8 TID = 10;

		gfx_handle_t dest_buffer = {};
		gfx_handle_t src_texture = {};
		u32			 src_layer	 = 0;
		u32			 src_mip	 = 0;
		vec2u_t		 size		 = vec2u_t::zero;
		u8			 bpp		 = 0;
	};

	struct command_copy_buffer_to_texture_t
	{
		static constexpr u8 TID = 11;

		texture_buffer_t* textures			  = nullptr;
		gfx_handle_t	  destination_texture = {};
		gfx_handle_t	  intermediate_buffer = {};
		u8				  mip_levels		  = 0;
		u8				  destination_slice	  = 0;
	};

	struct command_copy_buffer_region_to_texture_t
	{
		static constexpr u8 TID = 27;

		gfx_handle_t src_buffer	   = {};
		gfx_handle_t dst_texture   = {};
		u64			 src_offset	   = 0;
		u32			 src_row_pitch = 0;
		u16			 dst_x		   = 0;
		u16			 dst_y		   = 0;
		u16			 width		   = 0;
		u16			 height		   = 0;
		u8			 dst_mip	   = 0;
		u8			 bpp		   = 0;
	};

	struct command_copy_texture_to_texture_t
	{
		static constexpr u8 TID = 12;

		gfx_handle_t source					= {};
		gfx_handle_t destination			= {};
		u8			 source_layer			= 0;
		u8			 destination_layer		= 0;
		u8			 source_mip				= 0;
		u8			 source_total_mips		= 0;
		u8			 destination_mip		= 0;
		u8			 destination_total_mips = 0;
	};

	struct command_bind_vertex_buffers_t
	{
		static constexpr u8 TID = 13;

		gfx_handle_t buffer		 = {};
		u8			 slot		 = 0;
		u16			 vertex_size = 0;
		u64			 offset		 = 0;
	};

	struct command_bind_index_buffers_t
	{
		static constexpr u8 TID = 14;

		gfx_handle_t buffer		= {};
		u64			 offset		= 0;
		u8			 index_size = 0;
	};

	struct command_bind_group_t
	{
		static constexpr u8 TID = 15;

		gfx_handle_t group = {};
	};

	struct command_bind_constants_t
	{
		static constexpr u8 TID = 16;

		const void* data		= nullptr;
		u16			offset		= 0;
		u8			count		= 0;
		u8			param_index = 0;
	};

	struct command_dispatch_t
	{
		static constexpr u8 TID = 17;

		u32 group_size_x = 0;
		u32 group_size_y = 0;
		u32 group_size_z = 0;
	};

	struct command_barrier_t
	{
		static constexpr u8 TID = 18;

		const barrier_t* barriers	   = nullptr;
		u16				 barrier_count = 0;
	};

	struct command_begin_render_pass_depth_t
	{
		static constexpr u8 TID = 19;

		const render_pass_color_attachment_t*  color_attachments		= {};
		render_pass_depth_stencil_attachment_t depth_stencil_attachment = {};
		u8									   color_attachment_count	= 0;
	};

	struct command_begin_render_pass_swapchain_t
	{
		static constexpr u8 TID = 20;

		render_pass_swapchain_attachment_t* color_attachments	   = {};
		u8									color_attachment_count = 0;
	};

	struct command_begin_render_pass_swapchain_depth_t
	{
		static constexpr u8 TID = 21;

		render_pass_swapchain_attachment_t*	   color_attachments		= {};
		render_pass_depth_stencil_attachment_t depth_stencil_attachment = {};
		u8									   color_attachment_count	= 0;
	};

	struct command_bind_pipeline_compute_t
	{
		static constexpr u8 TID = 22;

		gfx_handle_t pipeline = {};
	};

	struct command_bind_layout_t
	{
		static constexpr u8 TID = 23;

		gfx_handle_t layout = {};
	};

	struct command_bind_layout_compute_t
	{
		static constexpr u8 TID = 24;

		gfx_handle_t layout = {};
	};

	struct command_copy_resource_region_t
	{
		static constexpr u8 TID = 25;

		gfx_handle_t source		 = {};
		gfx_handle_t destination = {};
		size_t		 dst_offset	 = 0;
		size_t		 src_offset	 = 0;
		size_t		 size		 = 0;
	};

	struct command_begin_render_pass_depth_only_t
	{
		static constexpr u8 TID = 26;

		render_pass_depth_stencil_attachment_t depth_stencil_attachment = {};
	};

}
