// Copyright (c) 2025 Inan Evin
#pragma once

#include "animation_common.hpp"
#include "animation_def.hpp"
#include "common_resources.hpp"

namespace sfg
{
	class istream_t;

	struct animation_channel_v3_runtime_t
	{
		const animation_keyframe_v3_t*		  keyframes		   = nullptr;
		const animation_keyframe_v3_spline_t* keyframes_spline = nullptr;
		u32									  keyframe_count   = 0;
		u32									  spline_count	   = 0;
		i32									  node_index	   = -1;
		animation_interpolation_e			  interpolation	   = animation_interpolation_e::linear;
	};

	struct animation_channel_q_runtime_t
	{
		const animation_keyframe_q_t*		 keyframes		  = nullptr;
		const animation_keyframe_q_spline_t* keyframes_spline = nullptr;
		u32									 keyframe_count	  = 0;
		u32									 spline_count	  = 0;
		i32									 node_index		  = -1;
		animation_interpolation_e			 interpolation	  = animation_interpolation_e::linear;
	};

	struct animation_runtime_t
	{
		animation_def_t						  def				= {};
		const animation_channel_v3_runtime_t* position_channels = nullptr;
		const animation_channel_q_runtime_t*  rotation_channels = nullptr;
		const animation_channel_v3_runtime_t* scale_channels	= nullptr;
		u32									  position_count	= 0;
		u32									  rotation_count	= 0;
		u32									  scale_count		= 0;
		f32									  duration			= 0.0f;
	};

	struct animation_internals_t
	{
		u32 reserved = 0;
	};

	class animation_loader_t
	{
	public:
		static constexpr u32 WIRE_MAGIC	  = make_resource_wire_magic('A', 'N', 'I', 'M');
		static constexpr u32 WIRE_VERSION = 1;

		static bool						 load(resource_entry_t& entry, resource_context_t& ctx);
		static create_internals_result_e create_internals(resource_entry_t& entry, resource_context_t& ctx);
		static void						 destroy_internals(resource_entry_t& entry, resource_context_t& ctx);
	};

	extern const resource_type_desc_t animation_resource_desc;
}
