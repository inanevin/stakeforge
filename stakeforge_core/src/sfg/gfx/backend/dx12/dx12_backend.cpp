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

#include "dx12_backend.hpp"
#include "sdk/d3d12.h"
#include "dx12_common.hpp"

// data
#include <sfg/data/static_vector.hpp>
#include <sfg/data/string_util.hpp>
#include <sfg/data/vector_util.hpp>

// gfx
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/gfx/common/texture_buffer.hpp>
#include <sfg/gfx/backend/dx12/sdk/D3D12MemAlloc.h>
#include <sfg/gfx/common/commands.hpp>

// misc
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/memory/memory_tracer.hpp>
#include <sfg/math/math_common.hpp>
#include <sfg/runtime/engine/engine_stats.hpp>

#ifdef SFG_DEBUG
#include <WinPixEventRuntime/pix3.h>
#endif

#ifdef SFG_GFX_SERIALIZE_SHADERS_PDB
#include <fstream>
#endif

#include <dxcapi.h>

using Microsoft::WRL::ComPtr;

namespace sfg
{

	Microsoft::WRL::ComPtr<IDxcLibrary> dx12_backend_t::s_idxcLib;

#define SFG_VERIFY_RENDER_NOT_RUNNING_OR_RENDER_THREAD() SFG_ASSERT(g_engine_runtime_stats.render_thread_id.load() == 0 || g_engine_runtime_stats.render_thread_id == static_cast<u64>(std::hash<std::thread::id>{}(std::this_thread::get_id())))

#define DX12_THROW(exception, ...)                                                                                                                                                                                                                                 \
	SFG_FATAL(__VA_ARGS__);                                                                                                                                                                                                                                        \
	throw exception;

#ifdef SFG_DEBUG
#define NAME_DX12_OBJECT_CSTR(x, NAME)                                                                                                                                                                                                                             \
	auto wcharConverted = string_util::char_to_wchar(NAME);                                                                                                                                                                                                        \
	x->SetName(wcharConverted);                                                                                                                                                                                                                                    \
	delete[] wcharConverted;

#define NAME_DX12_OBJECT(x, NAME) x->SetName(NAME)
#else
#define NAME_DX12_OBJECT_CSTR(x, NAME)
#define NAME_DX12_OBJECT(x, NAME)
#endif

	namespace
	{
		constexpr gfx_id_t INITIAL_RESOURCES		  = 1024;
		constexpr gfx_id_t INITIAL_TEXTURES			  = 1024;
		constexpr gfx_id_t INITIAL_SAMPLERS			  = 128;
		constexpr gfx_id_t INITIAL_SEMAPHORES		  = 64;
		constexpr gfx_id_t INITIAL_SHADERS			  = 2048;
		constexpr gfx_id_t INITIAL_PIPELINE_LAYOUTS	  = 256;
		constexpr gfx_id_t INITIAL_SWAPCHAINS		  = 8;
		constexpr gfx_id_t INITIAL_BIND_GROUPS		  = 512;
		constexpr gfx_id_t INITIAL_BIND_LAYOUTS		  = 128;
		constexpr gfx_id_t INITIAL_COMMAND_BUFFERS	  = 256;
		constexpr gfx_id_t INITIAL_QUEUES			  = 8;
		constexpr gfx_id_t INITIAL_DESCRIPTOR_HANDLES = 1024;

		D3D12_TEXTURE_ADDRESS_MODE get_address_mode(address_mode mode)
		{
			if (mode == address_mode::repeat)
				return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			else if (mode == address_mode::border)
				return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			else if (mode == address_mode::clamp)
				return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			else if (mode == address_mode::mirrored_clamp)
				return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
			else if (mode == address_mode::mirrored_repeat)
				return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;

			return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		}

		D3D12_FILTER get_filter(bitmask_t<u16> sampler_flags)
		{
			const bool compare = sampler_flags.is_set(sampler_flags::saf_compare);

			if (sampler_flags.is_set(saf_min_anisotropic) && sampler_flags.is_set(saf_mag_anisotropic))
				return compare ? D3D12_FILTER_COMPARISON_ANISOTROPIC : D3D12_FILTER_ANISOTROPIC;
			else if (sampler_flags.is_set(saf_mip_linear) && sampler_flags.is_set(saf_min_linear) && sampler_flags.is_set(saf_mag_linear))
				return compare ? D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR : D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			else if (sampler_flags.is_set(saf_mip_nearest) && sampler_flags.is_set(saf_min_linear) && sampler_flags.is_set(saf_mag_linear))
				return compare ? D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT : D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
			else if (sampler_flags.is_set(saf_mip_linear) && sampler_flags.is_set(saf_min_linear) && sampler_flags.is_set(saf_mag_nearest))
				return compare ? D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR : D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
			else if (sampler_flags.is_set(saf_mip_nearest) && sampler_flags.is_set(saf_min_linear) && sampler_flags.is_set(saf_mag_nearest))
				return compare ? D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT : D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
			else if (sampler_flags.is_set(saf_mip_linear) && sampler_flags.is_set(saf_min_nearest) && sampler_flags.is_set(saf_mag_linear))
				return compare ? D3D12_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR : D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR;
			else if (sampler_flags.is_set(saf_mip_nearest) && sampler_flags.is_set(saf_min_nearest) && sampler_flags.is_set(saf_mag_linear))
				return compare ? D3D12_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT : D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
			else if (sampler_flags.is_set(saf_mip_linear) && sampler_flags.is_set(saf_min_nearest) && sampler_flags.is_set(saf_mag_nearest))
				return compare ? D3D12_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR : D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
			else if (sampler_flags.is_set(saf_mip_nearest) && sampler_flags.is_set(saf_min_nearest) && sampler_flags.is_set(saf_mag_nearest))
				return compare ? D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT : D3D12_FILTER_MIN_MAG_MIP_POINT;

			return compare ? D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR : D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		}

		void border_color(bitmask_t<u16> flags, float* color_t)
		{
			if (flags.is_set(saf_border_transparent))
				color_t[0] = color_t[1] = color_t[2] = color_t[3] = 0.0f;
			else
				color_t[0] = color_t[1] = color_t[2] = color_t[3] = 1.0f;
		}

		D3D12_COMMAND_LIST_TYPE get_command_type(command_type type)
		{
			switch (type)
			{
			case command_type::graphics:
				return D3D12_COMMAND_LIST_TYPE_DIRECT;
			case command_type::transfer:
				return D3D12_COMMAND_LIST_TYPE_COPY;
			case command_type::compute:
				return D3D12_COMMAND_LIST_TYPE_COMPUTE;
			default:
				return D3D12_COMMAND_LIST_TYPE_DIRECT;
			}
		}

		DXGI_FORMAT get_format(format_e format)
		{
			switch (format)
			{
			case format_e::undefined:
				return DXGI_FORMAT_UNKNOWN;

				// 8 bit
			case format_e::r8_sint:
				return DXGI_FORMAT_R8_SINT;
			case format_e::r8_uint:
				return DXGI_FORMAT_R8_UINT;
			case format_e::r8_unorm:
				return DXGI_FORMAT_R8_UNORM;
			case format_e::r8_snorm:
				return DXGI_FORMAT_R8_SNORM;

			case format_e::r8g8_sint:
				return DXGI_FORMAT_R8G8_SINT;
			case format_e::r8g8_uint:
				return DXGI_FORMAT_R8G8_UINT;
			case format_e::r8g8_unorm:
				return DXGI_FORMAT_R8G8_UNORM;
			case format_e::r8g8_snorm:
				return DXGI_FORMAT_R8G8_SNORM;

			case format_e::r8g8b8a8_sint:
				return DXGI_FORMAT_R8G8B8A8_SINT;
			case format_e::r8g8b8a8_uint:
				return DXGI_FORMAT_R8G8B8A8_UINT;
			case format_e::r8g8b8a8_unorm:
				return DXGI_FORMAT_R8G8B8A8_UNORM;
			case format_e::r8g8b8a8_snorm:
				return DXGI_FORMAT_R8G8B8A8_SNORM;
			case format_e::r8g8b8a8_srgb:
				return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

			case format_e::b8g8r8a8_unorm:
				return DXGI_FORMAT_B8G8R8A8_UNORM;
			case format_e::b8g8r8a8_srgb:
				return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

				// 16 bit
			case format_e::r16_sint:
				return DXGI_FORMAT_R16_SINT;
			case format_e::r16_uint:
				return DXGI_FORMAT_R16_UINT;
			case format_e::r16_unorm:
				return DXGI_FORMAT_R16_UNORM;
			case format_e::r16_snorm:
				return DXGI_FORMAT_R16_SNORM;
			case format_e::r16_sfloat:
				return DXGI_FORMAT_R16_FLOAT;

			case format_e::r16g16_sint:
				return DXGI_FORMAT_R16G16_SINT;
			case format_e::r16g16_uint:
				return DXGI_FORMAT_R16G16_UINT;
			case format_e::r16g16_unorm:
				return DXGI_FORMAT_R16G16_UNORM;
			case format_e::r16g16_snorm:
				return DXGI_FORMAT_R16G16_SNORM;
			case format_e::r16g16_sfloat:
				return DXGI_FORMAT_R16G16_FLOAT;

			case format_e::r16g16b16a16_sint:
				return DXGI_FORMAT_R16G16B16A16_SINT;
			case format_e::r16g16b16a16_uint:
				return DXGI_FORMAT_R16G16B16A16_UINT;
			case format_e::r16g16b16a16_unorm:
				return DXGI_FORMAT_R16G16B16A16_UNORM;
			case format_e::r16g16b16a16_snorm:
				return DXGI_FORMAT_R16G16B16A16_SNORM;
			case format_e::r16g16b16a16_sfloat:
				return DXGI_FORMAT_R16G16B16A16_FLOAT;

				// 32 bit
			case format_e::r32_sint:
				return DXGI_FORMAT_R32_SINT;
			case format_e::r32_uint:
				return DXGI_FORMAT_R32_UINT;
			case format_e::r32_sfloat:
				return DXGI_FORMAT_R32_FLOAT;

			case format_e::r32g32_sint:
				return DXGI_FORMAT_R32G32_SINT;
			case format_e::r32g32_uint:
				return DXGI_FORMAT_R32G32_UINT;
			case format_e::r32g32_sfloat:
				return DXGI_FORMAT_R32G32_FLOAT;

			case format_e::r32g32b32_sfloat:
				return DXGI_FORMAT_R32G32B32_FLOAT;
			case format_e::r32g32b32_sint:
				return DXGI_FORMAT_R32G32B32_SINT;
			case format_e::r32g32b32_uint:
				return DXGI_FORMAT_R32G32B32_UINT;

			case format_e::r32g32b32a32_sint:
				return DXGI_FORMAT_R32G32B32A32_SINT;
			case format_e::r32g32b32a32_uint:
				return DXGI_FORMAT_R32G32B32A32_UINT;
			case format_e::r32g32b32a32_sfloat:
				return DXGI_FORMAT_R32G32B32A32_FLOAT;

				// depth-stencil
			case format_e::d32_sfloat:
				return DXGI_FORMAT_D32_FLOAT;
			case format_e::d24_unorm_s8_uint:
				return DXGI_FORMAT_D24_UNORM_S8_UINT;
			case format_e::d16_unorm:
				return DXGI_FORMAT_D16_UNORM;

				// misc
			case format_e::r11g11b10_sfloat:
				return DXGI_FORMAT_R11G11B10_FLOAT;
			case format_e::r10g0b10a2_int:
				return DXGI_FORMAT_R10G10B10A2_UINT;
			case format_e::r10g0b10a2_unorm:
				return DXGI_FORMAT_R10G10B10A2_UNORM;
			case format_e::bc3_block_srgb:
				return DXGI_FORMAT_BC3_UNORM_SRGB;
			case format_e::bc3_block_unorm:
				return DXGI_FORMAT_BC3_UNORM;

			default:
				return DXGI_FORMAT_UNKNOWN;
			}

			return DXGI_FORMAT_UNKNOWN;
		}

		u8 get_color_mask(bitmask_t<u8> comp_flags)
		{
			return comp_flags.value(); // 1-1 map
		}
		D3D12_BLEND get_blend(blend_factor factor)
		{
			switch (factor)
			{
			case blend_factor::zero:
				return D3D12_BLEND_ZERO;
			case blend_factor::one:
				return D3D12_BLEND_ONE;
			case blend_factor::src_color:
				return D3D12_BLEND_SRC_COLOR;
			case blend_factor::one_minus_src_color:
				return D3D12_BLEND_INV_SRC_COLOR;
			case blend_factor::dst_color:
				return D3D12_BLEND_DEST_COLOR;
			case blend_factor::one_minus_dst_color:
				return D3D12_BLEND_INV_DEST_COLOR;
			case blend_factor::src_alpha:
				return D3D12_BLEND_SRC_ALPHA;
			case blend_factor::one_minus_src_alpha:
				return D3D12_BLEND_INV_SRC_ALPHA;
			case blend_factor::dst_alpha:
				return D3D12_BLEND_DEST_ALPHA;
			case blend_factor::one_minus_dst_alpha:
				return D3D12_BLEND_INV_DEST_ALPHA;
			default:
				return D3D12_BLEND_ZERO;
			}
		}

		D3D12_BLEND_OP get_blend_op(blend_op op)
		{
			switch (op)
			{
			case blend_op::add:
				return D3D12_BLEND_OP_ADD;
			case blend_op::subtract:
				return D3D12_BLEND_OP_SUBTRACT;
			case blend_op::reverse_subtract:
				return D3D12_BLEND_OP_REV_SUBTRACT;
			case blend_op::min:
				return D3D12_BLEND_OP_MIN;
			case blend_op::max:
				return D3D12_BLEND_OP_MAX;
			default:
				return D3D12_BLEND_OP_ADD;
			}
		}

		D3D12_LOGIC_OP get_logic_op(logic_op op)
		{
			switch (op)
			{
			case logic_op::clear:
				return D3D12_LOGIC_OP_CLEAR;
			case logic_op::and_:
				return D3D12_LOGIC_OP_AND;
			case logic_op::and_reverse:
				return D3D12_LOGIC_OP_AND_REVERSE;
			case logic_op::copy:
				return D3D12_LOGIC_OP_COPY;
			case logic_op::and_inverted:
				return D3D12_LOGIC_OP_AND_INVERTED;
			case logic_op::no_op:
				return D3D12_LOGIC_OP_NOOP;
			case logic_op::xor_:
				return D3D12_LOGIC_OP_XOR;
			case logic_op::or_:
				return D3D12_LOGIC_OP_OR;
			case logic_op::nor:
				return D3D12_LOGIC_OP_NOR;
			case logic_op::equivalent:
				return D3D12_LOGIC_OP_EQUIV;
			default:
				return D3D12_LOGIC_OP_CLEAR;
			}
		}

		D3D12_CULL_MODE get_cull_mode(cull_mode cm)
		{
			switch (cm)
			{
			case cull_mode::none:
				return D3D12_CULL_MODE_NONE;
			case cull_mode::front:
				return D3D12_CULL_MODE_FRONT;
			case cull_mode::back:
				return D3D12_CULL_MODE_BACK;
			default:
				return D3D12_CULL_MODE_NONE;
			}
		}

		D3D12_COMPARISON_FUNC get_compare_op(compare_op op)
		{
			switch (op)
			{
			case compare_op::never:
				return D3D12_COMPARISON_FUNC_NEVER;
			case compare_op::less:
				return D3D12_COMPARISON_FUNC_LESS;
			case compare_op::equal:
				return D3D12_COMPARISON_FUNC_EQUAL;
			case compare_op::lequal:
				return D3D12_COMPARISON_FUNC_LESS_EQUAL;
			case compare_op::greater:
				return D3D12_COMPARISON_FUNC_GREATER;
			case compare_op::nequal:
				return D3D12_COMPARISON_FUNC_NOT_EQUAL;
			case compare_op::gequal:
				return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
			case compare_op::always:
				return D3D12_COMPARISON_FUNC_ALWAYS;
			default:
				return D3D12_COMPARISON_FUNC_ALWAYS;
			}
		}

		D3D12_STENCIL_OP get_stencil_op(stencil_op op)
		{
			switch (op)
			{
			case stencil_op::keep:
				return D3D12_STENCIL_OP_KEEP;
			case stencil_op::zero:
				return D3D12_STENCIL_OP_ZERO;
			case stencil_op::replace:
				return D3D12_STENCIL_OP_REPLACE;
			case stencil_op::increment_clamp:
				return D3D12_STENCIL_OP_INCR_SAT;
			case stencil_op::decrement_clamp:
				return D3D12_STENCIL_OP_DECR_SAT;
			case stencil_op::invert:
				return D3D12_STENCIL_OP_INVERT;
			case stencil_op::increment_wrap:
				return D3D12_STENCIL_OP_INCR;
			case stencil_op::decrement_wrap:
				return D3D12_STENCIL_OP_DECR;
			default:
				return D3D12_STENCIL_OP_KEEP;
			}
		}

		D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE get_load_op(load_op op)
		{
			switch (op)
			{
			case load_op::load:
				return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
			case load_op::clear:
				return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
			case load_op::dont_care:
				return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
			case load_op::none:
			default:
				return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS;
			}
		}

		D3D12_RENDER_PASS_ENDING_ACCESS_TYPE get_store_op(store_op op)
		{
			switch (op)
			{
			case store_op::store:
				return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
			case store_op::dont_care:
				return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
			case store_op::none:
			default:
				return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS;
			}
		}

		D3D12_FILL_MODE get_fill_mode(fill_mode fm)
		{
			switch (fm)
			{
			case fill_mode::wireframe:
				return D3D12_FILL_MODE_WIREFRAME;
			default:
				return D3D12_FILL_MODE_SOLID;
			}
		}

		D3D12_PRIMITIVE_TOPOLOGY_TYPE get_topology_type(topology tp)
		{
			switch (tp)
			{
			case topology::point_list:
				return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
			case topology::line_list:
			case topology::line_strip:
				return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
			case topology::triangle_list:
			case topology::triangle_strip:
			case topology::triangle_fan:
			default:
				return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			}
		}

		D3D12_PRIMITIVE_TOPOLOGY get_topology(topology tp)
		{
			switch (tp)
			{
			case topology::point_list:
				return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
			case topology::line_list:
				return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
			case topology::line_strip:
				return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
			case topology::triangle_list:
				return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			case topology::triangle_strip:
				return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
			case topology::triangle_fan:
				return D3D_PRIMITIVE_TOPOLOGY_TRIANGLEFAN;
			default:
				return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			}
		}

		D3D12_RESOURCE_STATES get_resource_state(u32 states)
		{
			bitmask_t<u32>		  mask			= states;
			D3D12_RESOURCE_STATES target_states = {};

			if (mask.is_set((u32)resource_state::resource_state_common))
				target_states |= D3D12_RESOURCE_STATE_COMMON;
			if (mask.is_set((u32)resource_state::resource_state_vertex_cbv))
				target_states |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
			if (mask.is_set((u32)resource_state::resource_state_index_buffer))
				target_states |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
			if (mask.is_set((u32)resource_state::resource_state_render_target))
				target_states |= D3D12_RESOURCE_STATE_RENDER_TARGET;
			if (mask.is_set((u32)resource_state::resource_state_uav))
				target_states |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			if (mask.is_set((u32)resource_state::resource_state_depth_write))
				target_states |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
			if (mask.is_set((u32)resource_state::resource_state_depth_read))
				target_states |= D3D12_RESOURCE_STATE_DEPTH_READ;
			if (mask.is_set((u32)resource_state::resource_state_non_ps_resource))
				target_states |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
			if (mask.is_set((u32)resource_state::resource_state_ps_resource))
				target_states |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			if (mask.is_set((u32)resource_state::resource_state_indirect_arg))
				target_states |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
			if (mask.is_set((u32)resource_state::resource_state_copy_dest))
				target_states |= D3D12_RESOURCE_STATE_COPY_DEST;
			if (mask.is_set((u32)resource_state::resource_state_copy_source))
				target_states |= D3D12_RESOURCE_STATE_COPY_SOURCE;
			if (mask.is_set((u32)resource_state::resource_state_resolve_dest))
				target_states |= D3D12_RESOURCE_STATE_RESOLVE_DEST;
			if (mask.is_set((u32)resource_state::resource_state_resolve_source))
				target_states |= D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
			if (mask.is_set((u32)resource_state::resource_state_generic_read))
				target_states |= D3D12_RESOURCE_STATE_GENERIC_READ;
			if (mask.is_set((u32)resource_state::resource_state_present))
				target_states |= D3D12_RESOURCE_STATE_PRESENT;

			return target_states;
		}

		D3D12_SHADER_VISIBILITY get_visibility(shader_stage stage)
		{
			if (stage == shader_stage::vertex)
				return D3D12_SHADER_VISIBILITY_VERTEX;
			else if (stage == shader_stage::fragment)
				return D3D12_SHADER_VISIBILITY_PIXEL;
			else
				return D3D12_SHADER_VISIBILITY_ALL;
		}

		void get_hw_adapter(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter, u32 gpu_type)
		{
			*ppAdapter = nullptr;

			ComPtr<IDXGIAdapter1> adapter;

			const unsigned int NVIDIA_VENDOR_ID = 0x10DE;
			const unsigned int AMD_VENDOR_ID	= 0x1002;
			const unsigned int INTEL_VENDOR_ID	= 0x8086;

			auto find = [&](bool agressive) {
				ComPtr<IDXGIFactory6> factory6;
				if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6))))
				{
					for (UINT index = 0; SUCCEEDED(factory6->EnumAdapterByGpuPreference(index, gpu_type == 0 ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_MINIMUM_POWER, IID_PPV_ARGS(&adapter))); ++index)
					{
						DXGI_ADAPTER_DESC1 desc;
						adapter->GetDesc1(&desc);

						if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
							continue;

						if (agressive && desc.VendorId != NVIDIA_VENDOR_ID && desc.VendorId != AMD_VENDOR_ID && gpu_type == 0)
							continue;

						if (agressive && (desc.VendorId == NVIDIA_VENDOR_ID || desc.VendorId == AMD_VENDOR_ID) && gpu_type == 1)
							continue;

						// Check to see whether the adapter supports Direct3D 12, but don't create the
						// actual device yet.
						if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
						{
							char* buf = string_util::wchar_to_char(desc.Description);
							SFG_INFO("selected hardware adapter {0}, dedicated video memory {1} mb", buf, desc.DedicatedVideoMemory * 0.000001);
							delete[] buf;
							break;
						}
					}
				}
			};

			find(true);
			if (adapter.Get() == nullptr)
				find(false);

			if (adapter.Get() == nullptr)
			{
				SFG_FATAL("failed finding a suitable device!");
				return;
			}

			*ppAdapter = adapter.Detach();
		}

		void msg_callback(D3D12_MESSAGE_CATEGORY msg_type, D3D12_MESSAGE_SEVERITY severity, D3D12_MESSAGE_ID messageId, LPCSTR pDesc, void* pContext)
		{
			if (pDesc != NULL)
			{
				if (severity == D3D12_MESSAGE_SEVERITY_MESSAGE)
				{
					SFG_INFO("{0}", pDesc);
				}
				else if (severity == D3D12_MESSAGE_SEVERITY_INFO)
				{
					// LOGV("{0}, pDesc);
				}
				else
				{
					SFG_ERR("{0}", pDesc);
				}
			}
		}

	}

	unique_t<dx12_backend_t> dx12_backend_t::s_instance = nullptr;

	bool dx12_backend_t::init_instance()
	{
		if (s_instance != nullptr)
			return false;

		s_instance				= make_unique<dx12_backend_t>();
		dx12_backend_t* backend = dx12_backend_t::get();
		if (!backend->init())
		{
			s_instance.reset();
			return false;
		}

		return true;
	}

	void dx12_backend_t::uninit_instance()
	{
		dx12_backend_t* backend = dx12_backend_t::get();
		backend->uninit();
		s_instance.reset();
	}

	DWORD msgcallback = 0;
	bool  dx12_backend_t::init()
	{
		UINT dxgiFactoryFlags = 0;

#ifdef SFG_GFX_USE_DEBUG_LAYERS
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();

			// Enable additional debug layers.
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
		else
		{
			SFG_ERR("Failed enabling debug layers!");
		}
#endif

		throw_if_failed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&_factory)));

		ComPtr<IDXGIFactory5> factory5;
		HRESULT				  facres = _factory.As(&factory5);
		if (SUCCEEDED(facres))
			facres = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &_tearing_supported, sizeof(_tearing_supported));

		// Choose gpu & create device
		{
			get_hw_adapter(_factory.Get(), &_adapter, GPU_DEVICE);
			throw_if_failed(D3D12CreateDevice(_adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&_device)));
		}

#ifdef SFG_GFX_USE_DEBUG_LAYERS
		// Dbg callback
		{
			ID3D12InfoQueue1* infoQueue = nullptr;
			if (SUCCEEDED(_device->QueryInterface<ID3D12InfoQueue1>(&infoQueue)))
			{
				if (infoQueue)
					infoQueue->RegisterMessageCallback(&msg_callback, D3D12_MESSAGE_CALLBACK_IGNORE_FILTERS, nullptr, &msgcallback);
			}
		}
#endif

		D3D12_FEATURE_DATA_D3D12_OPTIONS opts = {};
		if (!SUCCEEDED(_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &opts, sizeof(opts))))
		{
			SFG_ERR("failed checking device options!");
			return false;
		}

		D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = {};
		shaderModel.HighestShaderModel				= D3D_SHADER_MODEL_6_6; // ask for highest you care about

		HRESULT hr = _device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel));

		if (!SUCCEEDED(hr))
		{
			SFG_ERR("shader model 6.6 is not supported on this device!");
			return false;
		}

		if (opts.ResourceBindingTier != D3D12_RESOURCE_BINDING_TIER_3)
		{
			SFG_ERR("GPU device does not support resource binding tier 3!");
			return false;
		}

		// Allocator
		{
			D3D12MA::ALLOCATOR_DESC allocatorDesc;
			allocatorDesc.pDevice			   = _device.Get();
			allocatorDesc.PreferredBlockSize   = 0;
			allocatorDesc.Flags				   = D3D12MA::ALLOCATOR_FLAG_NONE;
			allocatorDesc.pAdapter			   = _adapter.Get();
			allocatorDesc.pAllocationCallbacks = NULL;
			throw_if_failed(D3D12MA::CreateAllocator(&allocatorDesc, &_allocator));
		}

		// DXC
		{
			HRESULT hr = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&s_idxcLib));
			if (FAILED(hr))
			{
				SFG_ERR("failed to create DXC library!");
				return false;
			}
		}

		_resources.reserve(INITIAL_RESOURCES);
		_textures.reserve(INITIAL_TEXTURES);
		_texture_shared_handles.reserve(INITIAL_TEXTURES);
		_samplers.reserve(INITIAL_SAMPLERS);
		_swapchains.reserve(INITIAL_SWAPCHAINS);
		_semaphores.reserve(INITIAL_SEMAPHORES);
		_shaders.reserve(INITIAL_SHADERS);
		_bind_groups.reserve(INITIAL_BIND_GROUPS);
		_command_buffers.reserve(INITIAL_COMMAND_BUFFERS);
		_command_allocators.reserve(INITIAL_COMMAND_BUFFERS);
		_queues.reserve(INITIAL_QUEUES);
		_indirect_signatures.reserve(INITIAL_PIPELINE_LAYOUTS);
		_descriptors.reserve(INITIAL_DESCRIPTOR_HANDLES);
		_bind_layouts.reserve(INITIAL_BIND_LAYOUTS);

		_queue_graphics = create_queue({
			.type		= command_type::graphics,
			.debug_name = {"GfxQueue"},
		});
		_queue_transfer = create_queue({
			.type		= command_type::transfer,
			.debug_name = {"TransferQueue"},
		});
		_queue_compute	= create_queue({
			 .type		 = command_type::compute,
			 .debug_name = {"CmpQueue"},
		 });

		const u32 size_cbv_srv_uav = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		const u32 size_dsv		   = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		const u32 size_rtv		   = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		const u32 size_sampler	   = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

		constexpr size_t count_dsv		   = 1024;
		constexpr size_t count_rtv		   = 1024;
		constexpr size_t count_cbv_srv_uav = 1024;
		constexpr size_t count_sampler	   = 1024;

		_heap_dsv.init(_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, count_dsv, size_dsv, false);
		_heap_rtv.init(_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, count_rtv, size_rtv, false);
		_heap_gpu_buffer.init(_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, count_cbv_srv_uav, size_cbv_srv_uav, true);
		_heap_gpu_sampler.init(_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, count_sampler, size_sampler, true);

		_reuse_dest_descriptors_buffer.reserve(100);
		_reuse_dest_descriptors_sampler.reserve(100);
		_reuse_src_descriptors_buffer.reserve(100);
		_reuse_src_descriptors_sampler.reserve(100);
		_reuse_root_params.reserve(100);
		_reuse_root_ranges.reserve(100);
		_reuse_static_samplers.reserve(100);
		return true;
	}

	void dx12_backend_t::uninit()
	{
		destroy_queue(_queue_graphics);
		destroy_queue(_queue_transfer);
		destroy_queue(_queue_compute);

		SFG_ASSERT(_resources.empty());
		SFG_ASSERT(_textures.empty());
		SFG_ASSERT(_samplers.empty());
		SFG_ASSERT(_swapchains.empty());
		SFG_ASSERT(_semaphores.empty());
		SFG_ASSERT(_shaders.empty());
		SFG_ASSERT(_bind_groups.empty());
		SFG_ASSERT(_command_buffers.empty());
		SFG_ASSERT(_command_allocators.empty());
		SFG_ASSERT(_queues.empty());
		SFG_ASSERT(_indirect_signatures.empty());
		SFG_ASSERT(_descriptors.empty());
		SFG_ASSERT(_bind_layouts.empty());

		_heap_dsv.uninit();
		_heap_rtv.uninit();
		_heap_gpu_buffer.uninit();
		_heap_gpu_sampler.uninit();

#ifdef SFG_GFX_USE_DEBUG_LAYERS
		ID3D12InfoQueue1* infoQueue = nullptr;
		if (SUCCEEDED(_device->QueryInterface<ID3D12InfoQueue1>(&infoQueue)))
		{
			infoQueue->UnregisterMessageCallback(msgcallback);
		}
#endif

		_allocator->Release();
		s_idxcLib.Reset();
		_device.Reset();
	}

	void dx12_backend_t::reset_command_buffer(gfx_command_buffer_handle cmd_buffer)
	{
		command_buffer_t&			cmd_buf	  = _command_buffers.get(cmd_buffer);
		ID3D12GraphicsCommandList4* cmd_list  = cmd_buf.ptr.Get();
		ID3D12CommandAllocator*		cmd_alloc = _command_allocators.get(cmd_buf.allocator).ptr.Get();
		throw_if_failed(cmd_alloc->Reset());
		throw_if_failed(cmd_list->Reset(cmd_alloc, nullptr));
		ID3D12DescriptorHeap* heaps[] = {_heap_gpu_buffer.get_heap(), _heap_gpu_sampler.get_heap()};
		cmd_list->SetDescriptorHeaps(_countof(heaps), heaps);
	}

	void dx12_backend_t::reset_command_buffer_transfer(gfx_command_buffer_handle cmd_buffer)
	{
		command_buffer_t&			cmd_buf	  = _command_buffers.get(cmd_buffer);
		ID3D12GraphicsCommandList4* cmd_list  = cmd_buf.ptr.Get();
		ID3D12CommandAllocator*		cmd_alloc = _command_allocators.get(cmd_buf.allocator).ptr.Get();
		throw_if_failed(cmd_alloc->Reset());
		throw_if_failed(cmd_list->Reset(cmd_alloc, nullptr));
	}

	void dx12_backend_t::close_command_buffer(gfx_command_buffer_handle cmd_buffer)
	{
		command_buffer_t&			cmd_buf	 = _command_buffers.get(cmd_buffer);
		ID3D12GraphicsCommandList4* cmd_list = cmd_buf.ptr.Get();
		throw_if_failed(cmd_list->Close());
	}

	void dx12_backend_t::submit_commands(gfx_queue_handle queue_id, const gfx_command_buffer_handle* commands, u8 commands_count)
	{
		queue_t&								q = _queues.get(queue_id);
		static_vector_t<ID3D12CommandList*, 32> lists;
		lists.resize(0);

		for (u8 i = 0; i < commands_count; i++)
		{
			command_buffer_t& cb = _command_buffers.get(commands[i]);
			lists.push_back(cb.ptr.Get());
		}

		q.ptr->ExecuteCommandLists(static_cast<u32>(commands_count), lists.data());
	}

	void dx12_backend_t::queue_wait(gfx_queue_handle queue_id, const gfx_semaphore_handle* semaphores, const u64* semaphore_values, u8 semaphore_count)
	{
		queue_t& q = _queues.get(queue_id);

		for (u8 i = 0; i < semaphore_count; i++)
			q.ptr->Wait(_semaphores.get(semaphores[i]).ptr.Get(), semaphore_values[i]);
	}

	void dx12_backend_t::queue_signal(gfx_queue_handle queue_id, const gfx_semaphore_handle* semaphores, const u64* semaphore_values, u8 semaphore_count)
	{
		queue_t& q = _queues.get(queue_id);

		for (u8 i = 0; i < semaphore_count; i++)
		{
			q.ptr->Signal(_semaphores.get(semaphores[i]).ptr.Get(), semaphore_values[i]);
		}
	}

	void dx12_backend_t::present(const gfx_swapchain_handle* swapchains, u8 swapchain_count)
	{
		for (u8 i = 0; i < swapchain_count; i++)
		{
			swapchain_t&			swp	   = _swapchains.get(swapchains[i]);
			DXGI_PRESENT_PARAMETERS params = {};

			throw_if_failed(swp.ptr->Present1(swp.vsync, swp.tearing ? DXGI_PRESENT_ALLOW_TEARING : 0, &params));
			// swp.image_index = swp.ptr->GetCurrentBackBufferIndex();
		}
	}

	void dx12_backend_t::wait_for_swapchain_latency(gfx_swapchain_handle swapchain_id)
	{
#if USE_WAITABLE_SWAPCHAIN
		swapchain_t& swp = _swapchains.get(swapchain_id);
		if (swp.frame_latency_waitable != NULL)
		{
			WaitForSingleObject(swp.frame_latency_waitable, INFINITE);
		}
#endif
	}

	u8 dx12_backend_t::get_back_buffer_index(gfx_swapchain_handle s)
	{
		swapchain_t& swp = _swapchains.get(s);
		swp.image_index	 = swp.ptr->GetCurrentBackBufferIndex();
		return swp.image_index;
	}

	gfx_resource_handle dx12_backend_t::create_resource(const resource_desc_t& desc)
	{
		SFG_VERIFY_RENDER_NOT_RUNNING_OR_RENDER_THREAD();

		const gfx_resource_handle id  = _resources.add();
		resource_t&				  res = _resources.get(id);

		const u32 aligned_size = ALIGN_SIZE_POW(desc.size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
		res.size			   = desc.flags.is_set(resource_flags::rf_constant_buffer) ? aligned_size : desc.size;

		const D3D12_RESOURCE_DESC resource_desc_t = {
			.Dimension		  = D3D12_RESOURCE_DIMENSION_BUFFER,
			.Alignment		  = 0,
			.Width			  = static_cast<u64>(res.size),
			.Height			  = 1,
			.DepthOrArraySize = 1,
			.MipLevels		  = 1,
			.Format			  = DXGI_FORMAT_UNKNOWN,
			.SampleDesc =
				{
					.Count	 = 1,
					.Quality = 0,
				},
			.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
			.Flags	= desc.flags.is_set(resource_flags::rf_gpu_write) ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE,
		};

		D3D12MA::ALLOCATION_DESC allocation_desc = {};
		D3D12_RESOURCE_STATES	 state			 = {};

		if (desc.flags.is_set(resource_flags::rf_gpu_only))
		{
			allocation_desc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
			state					 = D3D12_RESOURCE_STATE_COMMON;

			// if (desc.flags.is_set(resource_flags::rf_vertex_buffer) || desc.flags.is_set(resource_flags::rf_constant_buffer))
			//	state = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
			// else if (desc.flags.is_set(resource_flags::rf_index_buffer))
			//	state = D3D12_RESOURCE_STATE_INDEX_BUFFER;
		}
		else if (desc.flags.is_set(resource_flags::rf_cpu_visible))
		{
			allocation_desc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
			state					 = D3D12_RESOURCE_STATE_GENERIC_READ;
		}
		else if (desc.flags.is_set(resource_flags::rf_readback))
		{
			allocation_desc.HeapType = D3D12_HEAP_TYPE_READBACK;
			state					 = D3D12_RESOURCE_STATE_COPY_DEST;
		}

		throw_if_failed(_allocator->CreateResource(&allocation_desc, &resource_desc_t, state, NULL, &res.ptr, IID_NULL, NULL));

		{
			SFG_MEMTRACE_SCOPE("GPU");
			SFG_MEMTRACE_ALLOC(res.ptr, desc.size);
		}

		{
			SFG_MEMTRACE_SCOPE("GPUR");
			SFG_MEMTRACE_ALLOC(res.ptr->GetResource(), desc.size);
		}

		if (desc.flags.is_set(resource_flags::rf_constant_buffer))
		{
			const D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {
				.BufferLocation = res.ptr->GetResource()->GetGPUVirtualAddress(),
				.SizeInBytes	= static_cast<UINT>(res.size),
			};
			res.descriptor_index	= _descriptors.add();
			descriptor_handle_t& dh = _descriptors.get(res.descriptor_index);
			dh						= _heap_gpu_buffer.get_heap_handle_block(1);
			_device->CreateConstantBufferView(&desc, {dh.cpu});
		}
		else
		{
			if (desc.flags.is_set(resource_flags::rf_storage_buffer))
			{
				const D3D12_SHADER_RESOURCE_VIEW_DESC srv = {
					.Format					 = desc.structure_size == 0 ? DXGI_FORMAT_R32_TYPELESS : DXGI_FORMAT_UNKNOWN,
					.ViewDimension			 = D3D12_SRV_DIMENSION_BUFFER,
					.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
					.Buffer =
						{
							.FirstElement		 = 0,
							.NumElements		 = desc.structure_count == 0 ? desc.size / 4 : desc.structure_count,
							.StructureByteStride = desc.structure_size,
							.Flags				 = desc.structure_size == 0 ? D3D12_BUFFER_SRV_FLAG_RAW : D3D12_BUFFER_SRV_FLAG_NONE,
						},

				};
				res.descriptor_index	= _descriptors.add();
				descriptor_handle_t& dh = _descriptors.get(res.descriptor_index);
				dh						= _heap_gpu_buffer.get_heap_handle_block(1);
				_device->CreateShaderResourceView(res.ptr->GetResource(), &srv, {dh.cpu});
			}

			if (desc.flags.is_set(resource_flags::rf_gpu_write))
			{
				const D3D12_UNORDERED_ACCESS_VIEW_DESC uav = {
					.Format		   = DXGI_FORMAT_UNKNOWN,
					.ViewDimension = D3D12_UAV_DIMENSION_BUFFER,
					.Buffer =
						{
							.FirstElement		 = 0,
							.NumElements		 = desc.structure_count == 0 ? desc.size / 4 : desc.structure_count,
							.StructureByteStride = desc.structure_size,
							.Flags				 = desc.structure_size == 0 ? D3D12_BUFFER_UAV_FLAG_RAW : D3D12_BUFFER_UAV_FLAG_NONE,
						},
				};

				if (res.descriptor_index.is_null())
				{
					res.descriptor_index	= _descriptors.add();
					descriptor_handle_t& dh = _descriptors.get(res.descriptor_index);
					dh						= _heap_gpu_buffer.get_heap_handle_block(1);
					_device->CreateUnorderedAccessView(res.ptr->GetResource(), NULL, &uav, {dh.cpu});
				}
				else
				{
					res.descriptor_index_secondary = _descriptors.add();
					descriptor_handle_t& dh		   = _descriptors.get(res.descriptor_index_secondary);
					dh							   = _heap_gpu_buffer.get_heap_handle_block(1);
					_device->CreateUnorderedAccessView(res.ptr->GetResource(), NULL, &uav, {dh.cpu});
				}
			}
		}

		NAME_DX12_OBJECT_CSTR(res.ptr->GetResource(), desc.debug_name);

		return id;
	}

	void dx12_backend_t::map_resource(gfx_resource_handle id, u8*& ptr) const
	{
		const resource_t& res = _resources.get(id);
		CD3DX12_RANGE	  range(0, 0);
		throw_if_failed(res.ptr->GetResource()->Map(0, &range, reinterpret_cast<void**>(&ptr)));
	}

	void dx12_backend_t::unmap_resource(gfx_resource_handle id) const
	{
		const resource_t& res = _resources.get(id);
		CD3DX12_RANGE	  range(0, 0);
		res.ptr->GetResource()->Unmap(0, &range);
	}

	HANDLE dx12_backend_t::get_shared_handle_for_texture(gfx_texture_handle id)
	{
		return _texture_shared_handles.get(_textures.get(id).shared_handle).handle;
	}

	void dx12_backend_t::destroy_resource(gfx_resource_handle id)
	{
		SFG_VERIFY_RENDER_NOT_RUNNING_OR_RENDER_THREAD();

		resource_t& res = _resources.get(id);

		{
			SFG_MEMTRACE_SCOPE("GPU");
			SFG_MEMTRACE_DEALLOC(res.ptr);
		}

		{
			SFG_MEMTRACE_SCOPE("GPUR");
			SFG_MEMTRACE_DEALLOC(res.ptr->GetResource());
		}

		if (!res.descriptor_index.is_null())
		{
			descriptor_handle_t& dh = _descriptors.get(res.descriptor_index);
			_heap_gpu_buffer.remove_handle(dh);
			_descriptors.remove(res.descriptor_index);
		}

		if (!res.descriptor_index_secondary.is_null())
		{
			descriptor_handle_t& dh = _descriptors.get(res.descriptor_index_secondary);
			_heap_gpu_buffer.remove_handle(dh);
			_descriptors.remove(res.descriptor_index_secondary);
		}

		res.descriptor_index = {};
		res.ptr->Release();
		_resources.remove(id);
	}

	gfx_texture_handle dx12_backend_t::create_texture(const texture_desc_t& desc)
	{
		SFG_VERIFY_RENDER_NOT_RUNNING_OR_RENDER_THREAD();

		const gfx_texture_handle id	 = _textures.add();
		texture_t&				 txt = _textures.get(id);

		const DXGI_FORMAT color_format = get_format(desc.texture_format);
		const DXGI_FORMAT depth_format = get_format(desc.depth_stencil_format);
		txt.format					   = static_cast<u8>(color_format);

		D3D12_RESOURCE_DESC resource_desc_t = {};
		resource_desc_t.Dimension			= D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resource_desc_t.Alignment			= 0;
		resource_desc_t.Width				= static_cast<u64>(desc.size.x);
		resource_desc_t.Height				= static_cast<u64>(desc.size.y);
		resource_desc_t.DepthOrArraySize	= static_cast<u16>(desc.array_length);
		resource_desc_t.MipLevels			= static_cast<u16>(desc.mip_levels);
		resource_desc_t.SampleDesc.Count	= static_cast<u32>(desc.samples);
		resource_desc_t.SampleDesc.Quality	= 0;
		resource_desc_t.Layout				= D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resource_desc_t.Flags				= D3D12_RESOURCE_FLAG_NONE;
		resource_desc_t.Format				= desc.flags.is_set(texture_flags::tf_typeless) ? DXGI_FORMAT_R32_TYPELESS : color_format;
		resource_desc_t.Dimension			= desc.flags.is_set(texture_flags::tf_is_1d) ? D3D12_RESOURCE_DIMENSION_TEXTURE1D : (desc.flags.is_set(texture_flags::tf_is_3d) ? D3D12_RESOURCE_DIMENSION_TEXTURE3D : D3D12_RESOURCE_DIMENSION_TEXTURE2D);

		D3D12_CLEAR_VALUE  clear_value	   = {};
		D3D12_CLEAR_VALUE* clear_value_ptr = nullptr;

		if (desc.flags.is_set(texture_flags::tf_depth_texture) || desc.flags.is_set(texture_flags::tf_stencil_texture))
		{
			clear_value.Format				 = depth_format;
			clear_value.DepthStencil.Depth	 = desc.clear_values[0];
			clear_value.DepthStencil.Stencil = desc.clear_values[1];
			clear_value_ptr					 = &clear_value;
			resource_desc_t.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		}

		if (desc.flags.is_set(texture_flags::tf_render_target))
		{
			resource_desc_t.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
			clear_value.Format	 = color_format;
			clear_value.Color[0] = desc.clear_values[0];
			clear_value.Color[1] = desc.clear_values[1];
			clear_value.Color[2] = desc.clear_values[2];
			clear_value.Color[3] = desc.clear_values[3];
			clear_value_ptr		 = &clear_value;
		}

		if (desc.samples == 1 && !desc.flags.is_set(texture_flags::tf_sampled) && !desc.flags.is_set(texture_flags::tf_sampled_outside_fragment) && (desc.flags.is_set(texture_flags::tf_depth_texture) || desc.flags.is_set(texture_flags::tf_stencil_texture)))
			resource_desc_t.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

		if (desc.flags.is_set(texture_flags::tf_gpu_write))
			resource_desc_t.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		const D3D12MA::ALLOCATION_DESC allocation_desc = {
			.HeapType		= D3D12_HEAP_TYPE_DEFAULT,
			.ExtraHeapFlags = desc.flags.is_set(texture_flags::tf_shared) ? D3D12_HEAP_FLAG_SHARED : D3D12_HEAP_FLAG_NONE,
		};

		const D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;

		const D3D12_RESOURCE_ALLOCATION_INFO& allocation_info = _device->GetResourceAllocationInfo(0, 1, &resource_desc_t);

		throw_if_failed(_allocator->CreateResource(&allocation_desc, &resource_desc_t, state, clear_value_ptr, &txt.ptr, IID_NULL, NULL));
		NAME_DX12_OBJECT_CSTR(txt.ptr->GetResource(), desc.debug_name);

#ifdef SFG_ENABLE_MEMORY_TRACER
		{
			SFG_MEMTRACE_SCOPE("GPU");
			SFG_MEMTRACE_ALLOC(txt.ptr, txt.ptr->GetSize());
		}

		{
			SFG_MEMTRACE_SCOPE("GPUT");
			SFG_MEMTRACE_ALLOC(txt.ptr->GetResource(), txt.ptr->GetSize());
		}
#endif

		if (desc.flags.is_set(texture_flags::tf_shared))
		{
			txt.shared_handle				= _texture_shared_handles.add();
			texture_shared_handle_t& handle = _texture_shared_handles.get(txt.shared_handle);

			HANDLE sharedHandle;

			ID3D12Resource* resource_t = txt.ptr->GetResource();

			HRESULT hr = _device->CreateSharedHandle(resource_t, NULL, GENERIC_ALL, NULL, &sharedHandle);

			if (SUCCEEDED(hr))
			{
				handle.handle = sharedHandle;
				SFG_INFO("created shared handle for: {0}", desc.debug_name);
			}
			else
			{
				throw_if_failed(hr);
			}
		}

		auto create_srv = [&](DXGI_FORMAT format, bool createForCubemap, u32 baseArrayLayer, u32 layerCount, u32 baseMipLevel, u32 mipLevels, const descriptor_handle_t& targetDescriptor) {
			D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
			srv_desc.Shader4ComponentMapping		 = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srv_desc.Format							 = format;

			if (createForCubemap)
			{
				srv_desc.ViewDimension				 = D3D12_SRV_DIMENSION_TEXTURECUBE;
				srv_desc.TextureCube.MipLevels		 = desc.mip_levels;
				srv_desc.TextureCube.MostDetailedMip = baseMipLevel;
				srv_desc.TextureCube.MipLevels		 = mipLevels;
			}
			else if (desc.flags.is_set(texture_flags::tf_is_1d) && desc.array_length == 1)
			{
				srv_desc.ViewDimension			   = D3D12_SRV_DIMENSION_TEXTURE1D;
				srv_desc.Texture1D.MipLevels	   = mipLevels;
				srv_desc.Texture1D.MostDetailedMip = baseMipLevel;
			}
			else if (desc.flags.is_set(texture_flags::tf_is_1d) && desc.array_length > 1)
			{
				srv_desc.ViewDimension					= D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
				srv_desc.Texture1DArray.MipLevels		= mipLevels;
				srv_desc.Texture1DArray.MostDetailedMip = baseMipLevel;
				srv_desc.Texture1DArray.ArraySize		= layerCount;
				srv_desc.Texture1DArray.FirstArraySlice = baseArrayLayer;
			}
			else if (desc.flags.is_set(texture_flags::tf_is_2d) && desc.array_length == 1)
			{
				srv_desc.ViewDimension			   = desc.samples > 1 ? D3D12_SRV_DIMENSION_TEXTURE2DMS : D3D12_SRV_DIMENSION_TEXTURE2D;
				srv_desc.Texture2D.MipLevels	   = mipLevels;
				srv_desc.Texture2D.MostDetailedMip = baseMipLevel;
			}
			else if (desc.flags.is_set(texture_flags::tf_is_2d) && desc.array_length > 1)
			{
				srv_desc.ViewDimension					= desc.samples > 1 ? D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY : D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
				srv_desc.Texture2DArray.MipLevels		= mipLevels;
				srv_desc.Texture2DArray.MostDetailedMip = baseMipLevel;
				srv_desc.Texture2DArray.ArraySize		= layerCount;
				srv_desc.Texture2DArray.FirstArraySlice = baseArrayLayer;
			}
			else if (desc.flags.is_set(texture_flags::tf_is_3d))
			{
				srv_desc.ViewDimension			   = D3D12_SRV_DIMENSION_TEXTURE3D;
				srv_desc.Texture3D.MipLevels	   = mipLevels;
				srv_desc.Texture3D.MostDetailedMip = baseMipLevel;
			}

			_device->CreateShaderResourceView(txt.ptr->GetResource(), &srv_desc, {targetDescriptor.cpu});
		};

		auto create_rtv = [&](DXGI_FORMAT format, u32 baseArrayLayer, u32 layerCount, u32 baseMipLevel, u32 mipLevels, const descriptor_handle_t& targetDescriptor) {
			D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
			rtv_desc.Format						   = format;

			if (desc.flags.is_set(texture_flags::tf_is_1d) && desc.array_length == 1)
			{
				rtv_desc.ViewDimension		= D3D12_RTV_DIMENSION_TEXTURE1D;
				rtv_desc.Texture1D.MipSlice = baseMipLevel;
			}
			else if (desc.flags.is_set(texture_flags::tf_is_1d) && desc.array_length == 1)
			{
				rtv_desc.ViewDimension					= D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
				rtv_desc.Texture1DArray.ArraySize		= layerCount;
				rtv_desc.Texture1DArray.FirstArraySlice = baseArrayLayer;
				rtv_desc.Texture1DArray.MipSlice		= baseMipLevel;
			}
			else if (desc.flags.is_set(texture_flags::tf_is_2d) && desc.array_length == 1)
			{
				rtv_desc.ViewDimension		= desc.samples > 1 ? D3D12_RTV_DIMENSION_TEXTURE2DMS : D3D12_RTV_DIMENSION_TEXTURE2D;
				rtv_desc.Texture2D.MipSlice = baseMipLevel;
			}
			else if (desc.flags.is_set(texture_flags::tf_is_2d) && desc.array_length > 1)
			{
				rtv_desc.ViewDimension					= desc.samples > 1 ? D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY : D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
				rtv_desc.Texture2DArray.ArraySize		= layerCount;
				rtv_desc.Texture2DArray.FirstArraySlice = baseArrayLayer;
				rtv_desc.Texture2DArray.MipSlice		= baseMipLevel;
			}
			else if (desc.flags.is_set(texture_flags::tf_is_3d))
			{
				rtv_desc.ViewDimension		= D3D12_RTV_DIMENSION_TEXTURE3D;
				rtv_desc.Texture3D.MipSlice = baseMipLevel;
			}

			_device->CreateRenderTargetView(txt.ptr->GetResource(), &rtv_desc, {targetDescriptor.cpu});
		};

		auto create_dsv = [&](DXGI_FORMAT format, u32 baseArrayLayer, u32 layerCount, u32 baseMipLevel, u32 mipLevels, const descriptor_handle_t& targetDescriptor, bool read_only) {
			D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
			depthStencilDesc.Format						   = format;
			depthStencilDesc.Flags						   = read_only ? D3D12_DSV_FLAGS::D3D12_DSV_FLAG_READ_ONLY_DEPTH : D3D12_DSV_FLAGS::D3D12_DSV_FLAG_NONE;

			if (desc.flags.is_set(texture_flags::tf_is_1d) && desc.array_length == 1)
			{
				depthStencilDesc.ViewDimension		= D3D12_DSV_DIMENSION_TEXTURE1D;
				depthStencilDesc.Texture1D.MipSlice = baseMipLevel;
			}
			else if (desc.flags.is_set(texture_flags::tf_is_1d) && desc.array_length == 1)
			{
				depthStencilDesc.ViewDimension					= D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
				depthStencilDesc.Texture1DArray.ArraySize		= layerCount;
				depthStencilDesc.Texture1DArray.FirstArraySlice = baseArrayLayer;
				depthStencilDesc.Texture1DArray.MipSlice		= baseMipLevel;
			}
			else if (desc.flags.is_set(texture_flags::tf_is_2d) && desc.array_length == 1)
			{
				depthStencilDesc.ViewDimension		= desc.samples > 1 ? D3D12_DSV_DIMENSION_TEXTURE2DMS : D3D12_DSV_DIMENSION_TEXTURE2D;
				depthStencilDesc.Texture2D.MipSlice = baseMipLevel;
			}
			else if (desc.flags.is_set(texture_flags::tf_is_2d) && desc.array_length > 1)
			{
				depthStencilDesc.ViewDimension					= desc.samples > 1 ? D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY : D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
				depthStencilDesc.Texture2DArray.ArraySize		= layerCount;
				depthStencilDesc.Texture2DArray.FirstArraySlice = baseArrayLayer;
				depthStencilDesc.Texture2DArray.MipSlice		= baseMipLevel;
			}
			else if (desc.flags.is_set(texture_flags::tf_is_3d))
			{
				SFG_ASSERT(false, "Can't create a depth Texture 3D!");
			}

			_device->CreateDepthStencilView(txt.ptr->GetResource(), &depthStencilDesc, {targetDescriptor.cpu});
		};

		auto create_uav = [&](DXGI_FORMAT format, u32 baseArrayLayer, u32 layerCount, u32 baseMipLevel, u32 mipLevels, const descriptor_handle_t& targetDescriptor) {
			D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
			uav_desc.Format							  = format;

			if (desc.flags.is_set(texture_flags::tf_is_1d) && desc.array_length == 1)
			{
				uav_desc.ViewDimension		= D3D12_UAV_DIMENSION_TEXTURE1D;
				uav_desc.Texture1D.MipSlice = baseMipLevel;
			}
			else if (desc.flags.is_set(texture_flags::tf_is_1d) && desc.array_length == 1)
			{
				uav_desc.ViewDimension					= D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
				uav_desc.Texture1DArray.ArraySize		= layerCount;
				uav_desc.Texture1DArray.FirstArraySlice = baseArrayLayer;
				uav_desc.Texture1DArray.MipSlice		= baseMipLevel;
			}
			else if (desc.flags.is_set(texture_flags::tf_is_2d) && desc.array_length == 1)
			{
				uav_desc.ViewDimension		= desc.samples > 1 ? D3D12_UAV_DIMENSION_TEXTURE2DMS : D3D12_UAV_DIMENSION_TEXTURE2D;
				uav_desc.Texture2D.MipSlice = baseMipLevel;
			}
			else if (desc.flags.is_set(texture_flags::tf_is_2d) && desc.array_length > 1)
			{
				uav_desc.ViewDimension					= desc.samples > 1 ? D3D12_UAV_DIMENSION_TEXTURE2DMSARRAY : D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
				uav_desc.Texture2DArray.ArraySize		= layerCount;
				uav_desc.Texture2DArray.FirstArraySlice = baseArrayLayer;
				uav_desc.Texture2DArray.MipSlice		= baseMipLevel;
			}
			else if (desc.flags.is_set(texture_flags::tf_is_3d))
			{
				uav_desc.ViewDimension		= D3D12_UAV_DIMENSION_TEXTURE3D;
				uav_desc.Texture3D.MipSlice = baseMipLevel;
			}

			_device->CreateUnorderedAccessView(txt.ptr->GetResource(), NULL, &uav_desc, {targetDescriptor.cpu});
		};

		txt.view_count = static_cast<u8>(desc.views.size());

		for (u8 i = 0; i < txt.view_count; i++)
		{
			const view_desc_t& view = desc.views[i];
			texture_view_t&	   v	= txt.views[i];
			v.type					= static_cast<u8>(view.type);
			v.handle				= _descriptors.add();

			const u32 base_level	  = view.base_arr_level;
			const u32 remaining_level = view.level_count == 0 ? (desc.array_length - base_level) : view.level_count;
			const u32 base_mip		  = view.base_mip_level;
			const u32 remaining_mip	  = view.mip_count == 0 ? (desc.mip_levels - base_mip) : view.mip_count;

			descriptor_handle_t& dh = _descriptors.get(v.handle);

			if (view.type == view_type::sampled)
			{
				dh = _heap_gpu_buffer.get_heap_handle_block(1);
				create_srv(color_format, view.is_cubemap, base_level, remaining_level, base_mip, remaining_mip, dh);
			}
			else if (view.type == view_type::depth_stencil)
			{
				dh = _heap_dsv.get_heap_handle_block(1);
				create_dsv(depth_format, base_level, remaining_level, base_mip, remaining_mip, dh, view.read_only);
			}
			else if (view.type == view_type::render_target)
			{
				dh = _heap_rtv.get_heap_handle_block(1);
				create_rtv(color_format, base_level, remaining_level, base_mip, remaining_mip, dh);
			}
			else if (view.type == view_type::gpu_write)
			{
				dh = _heap_gpu_buffer.get_heap_handle_block(1);
				create_uav(color_format, base_level, remaining_level, base_mip, remaining_mip, dh);
			}
		}

		return id;
	}

	void dx12_backend_t::destroy_texture(gfx_texture_handle id)
	{
		SFG_VERIFY_RENDER_NOT_RUNNING_OR_RENDER_THREAD();

		texture_t& txt = _textures.get(id);

		{
			SFG_MEMTRACE_SCOPE("GPU");
			SFG_MEMTRACE_DEALLOC(txt.ptr);
		}

		{
			SFG_MEMTRACE_SCOPE("GPUT");
			SFG_MEMTRACE_DEALLOC(txt.ptr->GetResource());
		}

		if (!txt.shared_handle.is_null())
		{
			texture_shared_handle_t& handle = _texture_shared_handles.get(txt.shared_handle);
			if (handle.handle != 0)
				CloseHandle(handle.handle);
			handle.handle = 0;
			_texture_shared_handles.remove(txt.shared_handle);
			txt.shared_handle = {};
		}

		for (u8 i = 0; i < txt.view_count; i++)
		{
			texture_view_t&			   v  = txt.views[i];
			const descriptor_handle_t& dh = _descriptors.get(v.handle);
			const view_type			   vt = static_cast<view_type>(v.type);

			if (vt == view_type::render_target)
				_heap_rtv.remove_handle(dh);
			else if (vt == view_type::sampled || vt == view_type::gpu_write)
				_heap_gpu_buffer.remove_handle(dh);
			else if (vt == view_type::depth_stencil)
				_heap_dsv.remove_handle(dh);
			else
			{
				SFG_ASSERT(false);
			}

			_descriptors.remove(v.handle);
		}

		txt.ptr->Release();
		txt.ptr = NULL;
		_textures.remove(id);
	}

	gfx_sampler_handle dx12_backend_t::create_sampler(const sampler_desc_t& desc)
	{
		SFG_VERIFY_RENDER_NOT_RUNNING_OR_RENDER_THREAD();

		D3D12_SAMPLER_DESC samplerDesc = {
			.Filter			= get_filter(desc.flags),
			.AddressU		= get_address_mode(desc.address_u),
			.AddressV		= get_address_mode(desc.address_v),
			.AddressW		= get_address_mode(desc.address_w),
			.MipLODBias		= static_cast<FLOAT>(desc.lod_bias),
			.MaxAnisotropy	= desc.anisotropy,
			.ComparisonFunc = desc.flags.is_set(sampler_flags::saf_compare) ? get_compare_op(desc.compare) : D3D12_COMPARISON_FUNC_NONE,
			.MinLOD			= desc.min_lod,
			.MaxLOD			= desc.max_lod,
		};
		border_color(desc.flags, samplerDesc.BorderColor);

		const gfx_sampler_handle id	 = _samplers.add();
		sampler_t&				 smp = _samplers.get(id);

		smp.descriptor_index	= _descriptors.add();
		descriptor_handle_t& dh = _descriptors.get(smp.descriptor_index);
		dh						= _heap_gpu_sampler.get_heap_handle_block(1);

		_device->CreateSampler(&samplerDesc, {dh.cpu});
		return id;
	}

	void dx12_backend_t::destroy_sampler(gfx_sampler_handle id)
	{
		SFG_VERIFY_RENDER_NOT_RUNNING_OR_RENDER_THREAD();

		sampler_t&			 smp = _samplers.get(id);
		descriptor_handle_t& dh	 = _descriptors.get(smp.descriptor_index);
		_heap_gpu_sampler.remove_handle(dh);
		_descriptors.remove(smp.descriptor_index);
		_samplers.remove(id);
	}

	gfx_swapchain_handle dx12_backend_t::create_swapchain(const swapchain_desc_t& desc)
	{
		SFG_VERIFY_RENDER_NOT_RUNNING_OR_RENDER_THREAD();

		const gfx_swapchain_handle id  = _swapchains.add();
		swapchain_t&			   swp = _swapchains.get(id);

		if (desc.flags.is_set(swapchain_flags::sf_vsync_every_v_blank))
			swp.vsync = 1;
		else if (desc.flags.is_set(swapchain_flags::sf_vsync_every_2v_blank))
			swp.vsync = 2;
		else
			swp.vsync = 0;

		DXGI_FORMAT swap_format = DXGI_FORMAT_B8G8R8A8_UNORM;
		if (desc.format == format_e::r16g16b16a16_sfloat)
		{
			swap_format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		}
		else if (desc.format == format_e::r8g8b8a8_unorm)
		{
			swap_format = DXGI_FORMAT_R8G8B8A8_UNORM;
		}
		else if (desc.format == format_e::r8g8b8a8_srgb)
		{
			swap_format = DXGI_FORMAT_R8G8B8A8_UNORM;
		}

		// this is view(rtv) format_e
		swp.format = static_cast<u8>(get_format(desc.format));

		const bool vsync_on = desc.flags.is_set(swapchain_flags::sf_vsync_every_v_blank) || desc.flags.is_set(swapchain_flags::sf_vsync_every_2v_blank);
		const bool tearing	= _tearing_supported && !vsync_on && desc.flags.is_set(swapchain_flags::sf_allow_tearing);
		swp.tearing			= tearing;

		const DXGI_SWAP_CHAIN_DESC1 swapchain_desc_t = {
			.Width	= static_cast<UINT>(desc.size.x),
			.Height = static_cast<UINT>(desc.size.y),
			.Format = swap_format,
			.SampleDesc =
				{
					.Count = 1,
				},
			.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
			.BufferCount = BACK_BUFFER_COUNT,
			.SwapEffect	 = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL,
#if USE_WAITABLE_SWAPCHAIN
			.Flags = (tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : (UINT)0) | DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT,
#else
			.Flags = (tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : (UINT)0),
#endif
		};

		ComPtr<IDXGISwapChain1> swapchain_t;

		throw_if_failed(_factory->CreateSwapChainForHwnd(_queues.get(_queue_graphics).ptr.Get(), (HWND)desc.window_t, &swapchain_desc_t, nullptr, nullptr, &swapchain_t));
		throw_if_failed(swapchain_t.As(&swp.ptr));

#ifdef SFG_ENABLE_MEMORY_TRACER
		swp.size = desc.size.x * desc.size.y * 4;
		{
			SFG_MEMTRACE_SCOPE("Gfx");
			SFG_MEMTRACE_ALLOC(swp.ptr.Get(), swp.size);
		}
#endif

#if USE_WAITABLE_SWAPCHAIN
		Microsoft::WRL::ComPtr<IDXGISwapChain2> sc2;
		if (SUCCEEDED(swp.ptr.As(&sc2)))
		{
			sc2->SetMaximumFrameLatency(FRAME_LATENCY);
			swp.frame_latency_waitable = sc2->GetFrameLatencyWaitableObject();
		}
#endif

		{
			for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
			{
				throw_if_failed(swp.ptr->GetBuffer(i, IID_PPV_ARGS(&swp.textures[i])));
				swp.rtv_indices[i]		= _descriptors.add();
				descriptor_handle_t& dh = _descriptors.get(swp.rtv_indices[i]);
				dh						= _heap_rtv.get_heap_handle_block(1);

				const D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {
					.Format		   = static_cast<DXGI_FORMAT>(swp.format),
					.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
				};

				_device->CreateRenderTargetView(swp.textures[i].Get(), &rtv_desc, {dh.cpu});
				NAME_DX12_OBJECT_CSTR(swp.textures[i], "Swapchain RTV");
			}
		}

#ifdef SFG_ENABLE_MEMORY_TRACER
		{
			SFG_MEMTRACE_SCOPE("GfxTxt");
			SFG_MEMTRACE_ALLOC(swp.textures[0].Get(), swp.size);
		}
#endif

		// swp.image_index = swp.ptr->GetCurrentBackBufferIndex();
		return id;
	}

	gfx_swapchain_handle dx12_backend_t::recreate_swapchain(const swapchain_recreate_desc_t& desc)
	{
		SFG_VERIFY_RENDER_NOT_RUNNING_OR_RENDER_THREAD();

		swapchain_t& swp = _swapchains.get(desc.swapchain_t);

		DXGI_SWAP_CHAIN_DESC swp_desc = {};
		swp.ptr->GetDesc(&swp_desc);

#ifdef SFG_ENABLE_MEMORY_TRACER
		ID3D12Resource* old_swapchain_texture = swp.textures[0].Get();
#endif

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			swp.textures[i].Reset();
		}

		const bool vsync_on = desc.flags.is_set(swapchain_flags::sf_vsync_every_v_blank) || desc.flags.is_set(swapchain_flags::sf_vsync_every_2v_blank);
		const bool tearing	= _tearing_supported && !vsync_on && desc.flags.is_set(swapchain_flags::sf_allow_tearing);
		swp.tearing			= tearing;

		if (desc.flags.is_set(swapchain_flags::sf_vsync_every_v_blank))
			swp.vsync = 1;
		else if (desc.flags.is_set(swapchain_flags::sf_vsync_every_2v_blank))
			swp.vsync = 2;
		else
			swp.vsync = 0;

#ifdef SFG_ENABLE_MEMORY_TRACER

		const size_t new_size = desc.size.x * desc.size.y * 4;
		{
			SFG_MEMTRACE_SCOPE("GfxTxt");
			SFG_MEMTRACE_DEALLOC(old_swapchain_texture);
		}

		{
			SFG_MEMTRACE_SCOPE("Gfx");
			SFG_MEMTRACE_DEALLOC(swp.ptr.Get());
			SFG_MEMTRACE_ALLOC(swp.ptr.Get(), new_size);
		}

#endif

#if USE_WAITABLE_SWAPCHAIN
		UINT flags = (tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : (UINT)0) | DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
#else
		UINT flags = (tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : (UINT)0);
#endif

		throw_if_failed(swp.ptr->ResizeBuffers(BACK_BUFFER_COUNT, static_cast<UINT>(desc.size.x), static_cast<UINT>(desc.size.y), swp_desc.BufferDesc.Format, flags));
		// swp.image_index = swp.ptr->GetCurrentBackBufferIndex();

		// Re-apply frame latency settings and refresh waitable object after resize
		{
#if USE_WAITABLE_SWAPCHAIN
			Microsoft::WRL::ComPtr<IDXGISwapChain2> sc2;
			if (SUCCEEDED(swp.ptr.As(&sc2)))
			{
				sc2->SetMaximumFrameLatency(FRAME_LATENCY);
				swp.frame_latency_waitable = sc2->GetFrameLatencyWaitableObject();
			}
#endif
		}

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			descriptor_handle_t& dh = _descriptors.get(swp.rtv_indices[i]);

			throw_if_failed(swp.ptr->GetBuffer(i, IID_PPV_ARGS(&swp.textures[i])));

			const D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {
				.Format		   = static_cast<DXGI_FORMAT>(swp.format),
				.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
			};
			_device->CreateRenderTargetView(swp.textures[i].Get(), &rtv_desc, {dh.cpu});
			NAME_DX12_OBJECT_CSTR(swp.textures[i], "Swapchain RTV");
		}

#ifdef SFG_ENABLE_MEMORY_TRACER
		{
			SFG_MEMTRACE_SCOPE("GfxTxt");
			SFG_MEMTRACE_ALLOC(swp.textures[0].Get(), new_size);
		}
		swp.size = static_cast<u32>(new_size);
#endif

		return desc.swapchain_t;
	}

	void dx12_backend_t::destroy_swapchain(gfx_swapchain_handle id)
	{
		SFG_VERIFY_RENDER_NOT_RUNNING_OR_RENDER_THREAD();

		swapchain_t& swp = _swapchains.get(id);

#ifdef SFG_ENABLE_MEMORY_TRACER
		{
			SFG_MEMTRACE_SCOPE("Gfx");
			SFG_MEMTRACE_DEALLOC(swp.ptr.Get());
		}

		{
			SFG_MEMTRACE_SCOPE("GfxTxt");
			SFG_MEMTRACE_DEALLOC(swp.textures[0].Get());
		}
#endif

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			descriptor_handle_t& dh = _descriptors.get(swp.rtv_indices[i]);
			_heap_rtv.remove_handle(dh);
			_descriptors.remove(swp.rtv_indices[i]);
			swp.textures[i].Reset();
		}

		swp.ptr.Reset();
		_swapchains.remove(id);
	}

	gfx_semaphore_handle dx12_backend_t::create_semaphore()
	{
		SFG_VERIFY_RENDER_NOT_RUNNING_OR_RENDER_THREAD();

		const gfx_semaphore_handle id  = _semaphores.add();
		semaphore_t&			   sem = _semaphores.get(id);
		throw_if_failed(_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&sem.ptr)));
		return id;
	}

	void dx12_backend_t::wait_semaphore(gfx_semaphore_handle id, u64 value) const
	{
		const semaphore_t& sem = _semaphores.get(id);
		wait_for_fence(sem.ptr.Get(), value);
	}

	void dx12_backend_t::destroy_semaphore(gfx_semaphore_handle id)
	{
		SFG_VERIFY_RENDER_NOT_RUNNING_OR_RENDER_THREAD();

		semaphore_t& sem = _semaphores.get(id);
		sem.ptr.Reset();
		_semaphores.remove(id);
	}

	bool dx12_backend_t::compile_shader_vertex_pixel(u8 stage, const string_t& source, const vector_t<string_t>& defines, const vector_t<string_t>& source_paths, const char* entry_t, span_t<u8>& out, bool compile_root_sig, span_t<u8>& out_signature_data) const
	{
		Microsoft::WRL::ComPtr<IDxcCompiler3> idxc_compiler;
		throw_if_failed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&idxc_compiler)));

		ComPtr<IDxcUtils> utils;
		throw_if_failed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(utils.GetAddressOf())));

		UINT32					 code_page = CP_UTF8;
		ComPtr<IDxcBlobEncoding> source_blob;
		const char*				 shader_source = source.c_str();
		s_idxcLib->CreateBlobWithEncodingFromPinned((const BYTE*)shader_source, static_cast<UINT>(source.size()), code_page, &source_blob);

		DxcBuffer source_buffer;
		source_buffer.Ptr	   = source_blob->GetBufferPointer();
		source_buffer.Size	   = source_blob->GetBufferSize();
		source_buffer.Encoding = 0;

		vector_t<const wchar_t*> include_paths;
		include_paths.reserve(source_paths.size());
		for (const string_t& src_path : source_paths)
		{
			include_paths.push_back(string_util::char_to_wchar(src_path.c_str()));
		}

		auto compile = [&](const wchar_t* target_profile, const wchar_t* target_entry, span_t<u8>& out, bool root_sig) -> bool {
			std::vector<std::wstring> arg_storage;
			std::vector<LPCWSTR>	  arguments = {L"-T",
												   target_profile,
												   L"-E",
												   target_entry,
												   DXC_ARG_WARNINGS_ARE_ERRORS,
												   L"-HV",
												   L"2021",
#ifdef SFG_DEBUG
											  DXC_ARG_DEBUG,
											  DXC_ARG_PREFER_FLOW_CONTROL,
											  DXC_ARG_SKIP_OPTIMIZATIONS
#else
											  L"-Qstrip_debug",
											  L"-Qstrip_reflect",
											  DXC_ARG_OPTIMIZATION_LEVEL3
#endif
			};

			for (const auto& def : defines)
			{
				arg_storage.emplace_back(L"-D" + string_util::to_wstr(def));
				arguments.push_back(arg_storage.back().c_str());
			}

			for (const wchar_t* include : include_paths)
			{
				arguments.push_back(L"-I");
				arguments.push_back(include);
			}

			ComPtr<IDxcIncludeHandler> include_handler;
			throw_if_failed(utils->CreateDefaultIncludeHandler(&include_handler));

			ComPtr<IDxcResult> result;
			throw_if_failed(idxc_compiler->Compile(&source_buffer, arguments.data(), static_cast<u32>(arguments.size()), include_handler.Get(), IID_PPV_ARGS(result.GetAddressOf())));

			ComPtr<IDxcBlobUtf8> errors;
			result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(errors.GetAddressOf()), nullptr);

			if (errors && errors->GetStringLength() > 0)
			{
				SFG_ERR("{0}", (char*)errors->GetStringPointer());
				return false;
			}

			HRESULT hr;
			result->GetStatus(&hr);

			if (FAILED(hr))
			{
				if (result)
				{
					ComPtr<IDxcBlobEncoding> errorsBlob;
					hr = result->GetErrorBuffer(&errorsBlob);
					if (SUCCEEDED(hr) && errorsBlob)
					{
						SFG_FATAL("Shader compilation failed:{0}", (const char*)errorsBlob->GetBufferPointer());
						return false;
					}
				}
			}

#if SFG_GFX_SERIALIZE_SHADERS_PDB
			ComPtr<IDxcBlob>	  debug_data;
			ComPtr<IDxcBlobUtf16> debug_data_path;
			result->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(debug_data.GetAddressOf()), debug_data_path.GetAddressOf());

			if (debug_data != NULL && debug_data_path != NULL)
			{
				const wchar_t* path = reinterpret_cast<const wchar_t*>(debug_data_path->GetBufferPointer());

				wstring_t str = L"/_shaders_pdb/";
				str += path;

				if (debug_data && path)
				{
					std::ofstream out_file(path, std::ios::binary);
					out_file.write(reinterpret_cast<const char*>(debug_data->GetBufferPointer()), debug_data->GetBufferSize());
					out_file.close();
				}
			}

#endif

			if (root_sig)
			{
				ComPtr<IDxcBlob> root_sig_blob;
				result->GetOutput(DXC_OUT_ROOT_SIGNATURE, IID_PPV_ARGS(&root_sig_blob), nullptr);

				if (root_sig_blob && root_sig_blob->GetBufferSize() > 0)
				{
					out_signature_data.size = root_sig_blob->GetBufferSize();
					out_signature_data.data = new u8[out_signature_data.size];
					SFG_MEMCPY(out_signature_data.data, root_sig_blob->GetBufferPointer(), out_signature_data.size);
				}
			}
			ComPtr<IDxcBlob> code;
			result->GetResult(&code);

			if (code.Get() != NULL)
			{
				const SIZE_T sz = code->GetBufferSize();
				out.size		= code->GetBufferSize();
				out.data		= new u8[sz];
				SFG_MEMCPY(out.data, code->GetBufferPointer(), out.size);
			}
			else
			{
				SFG_FATAL("Failed compiling IDXC blob!");
				return false;
			}

			return true;
		};

		auto clean = [&]() {
			source_blob.Reset();
			for (const wchar_t* p : include_paths)
				delete[] p;
		};

		const wchar_t* target_entry = string_util::char_to_wchar(entry_t);

		const wchar_t* t = stage == shader_stage::vertex ? L"vs_6_6" : L"ps_6_6";
		if (!compile(t, target_entry, out, compile_root_sig))
		{
			delete[] target_entry;
			clean();
			return false;
		}

		delete[] target_entry;
		clean();
		return true;
	}

	bool dx12_backend_t::compile_shader_compute(const string_t& source, const vector_t<string_t>& source_paths, const char* entry_t, span_t<u8>& out, bool compile_layout, span_t<u8>& out_layout) const
	{
		Microsoft::WRL::ComPtr<IDxcCompiler3> idxc_compiler;
		throw_if_failed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&idxc_compiler)));

		ComPtr<IDxcUtils> utils;
		throw_if_failed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(utils.GetAddressOf())));

		UINT32					 code_page = CP_UTF8;
		ComPtr<IDxcBlobEncoding> source_blob;
		const char*				 shader_source = source.c_str();
		s_idxcLib->CreateBlobWithEncodingFromPinned((const BYTE*)shader_source, static_cast<UINT>(source.size()), code_page, &source_blob);

		DxcBuffer source_buffer;
		source_buffer.Ptr	   = source_blob->GetBufferPointer();
		source_buffer.Size	   = source_blob->GetBufferSize();
		source_buffer.Encoding = 0;

		const wchar_t* entry_point = string_util::char_to_wchar(entry_t);

		vector_t<LPCWSTR> arguments = {L"-T", L"cs_6_6", L"-E", entry_point, DXC_ARG_WARNINGS_ARE_ERRORS, L"-HV 2021"};

		vector_t<const wchar_t*> include_paths;
		include_paths.reserve(source_paths.size());
		for (const string_t& src_path : source_paths)
			include_paths.push_back(string_util::char_to_wchar(src_path.c_str()));

		for (const wchar_t* include : include_paths)
		{
			arguments.push_back(L"-I");
			arguments.push_back(include);
		}

#ifdef SFG_DEBUG
		arguments.push_back(DXC_ARG_DEBUG);
		arguments.push_back(DXC_ARG_PREFER_FLOW_CONTROL);
		arguments.push_back(DXC_ARG_SKIP_OPTIMIZATIONS);
#else
		arguments.push_back(L"-Qstrip_debug");
		arguments.push_back(L"-Qstrip_reflect");
		arguments.push_back(DXC_ARG_OPTIMIZATION_LEVEL3);
#endif

		ComPtr<IDxcIncludeHandler> include_handler;
		throw_if_failed(utils->CreateDefaultIncludeHandler(&include_handler));

		ComPtr<IDxcResult> result;
		throw_if_failed(idxc_compiler->Compile(&source_buffer, arguments.data(), static_cast<u32>(arguments.size()), include_handler.Get(), IID_PPV_ARGS(result.GetAddressOf())));

		delete[] entry_point;

		for (const wchar_t* p : include_paths)
			delete[] p;

		ComPtr<IDxcBlobUtf8> errors;
		result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(errors.GetAddressOf()), nullptr);
		if (errors && errors->GetStringLength() > 0)
		{
			SFG_ERR("{0}", (char*)errors->GetStringPointer());
			return false;
		}

		HRESULT hr;
		result->GetStatus(&hr);

		if (FAILED(hr))
		{
			if (result)
			{
				ComPtr<IDxcBlobEncoding> errorsBlob;
				hr = result->GetErrorBuffer(&errorsBlob);
				if (SUCCEEDED(hr) && errorsBlob)
				{
					SFG_FATAL("Shader compilation failed:{0}", (const char*)errorsBlob->GetBufferPointer());
					return false;
				}
			}
		}

#if SFG_GFX_SERIALIZE_SHADERS_PDB
		ComPtr<IDxcBlob>	  debug_data;
		ComPtr<IDxcBlobUtf16> debug_data_path;
		result->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(debug_data.GetAddressOf()), debug_data_path.GetAddressOf());

		if (debug_data != NULL && debug_data_path != NULL)
		{
			const wchar_t* path = reinterpret_cast<const wchar_t*>(debug_data_path->GetBufferPointer());

			if (debug_data && path)
			{
				std::ofstream out_file(path, std::ios::binary);
				out_file.write(reinterpret_cast<const char*>(debug_data->GetBufferPointer()), debug_data->GetBufferSize());
				out_file.close();
			}
		}

#endif

		if (compile_layout)
		{
			ComPtr<IDxcBlob> root_sig_blob;
			result->GetOutput(DXC_OUT_ROOT_SIGNATURE, IID_PPV_ARGS(&root_sig_blob), nullptr);

			if (root_sig_blob && root_sig_blob->GetBufferSize() > 0)
			{
				out_layout.size = root_sig_blob->GetBufferSize();
				out_layout.data = new u8[out_layout.size];
				SFG_MEMCPY(out_layout.data, root_sig_blob->GetBufferPointer(), out_layout.size);
			}
		}
		ComPtr<IDxcBlob> code;
		result->GetResult(&code);

		if (code.Get() != NULL)
		{
			const SIZE_T sz = code->GetBufferSize();
			out.size		= code->GetBufferSize();
			out.data		= new u8[sz];
			SFG_MEMCPY(out.data, code->GetBufferPointer(), out.size);
		}
		else
		{
			SFG_FATAL("Failed compiling IDXC blob!");
			return false;
		}

		source_blob.Reset();

		return true;
	}

	u32 dx12_backend_t::get_resource_gpu_index(gfx_resource_handle id, bool use_secondary)
	{
		const resource_t& r = _resources.get(id);
		if (r.descriptor_index.is_null())
			return UINT32_MAX;

		if (use_secondary)
			return _descriptors.get(r.descriptor_index_secondary).index;

		return _descriptors.get(r.descriptor_index).index;
	}

	u32 dx12_backend_t::get_texture_gpu_index(gfx_texture_handle id, u8 view_index)
	{
		const texture_t& t = _textures.get(id);
		SFG_ASSERT(t.view_count > view_index);
		return _descriptors.get(t.views[view_index].handle).index;
	}

	u32 dx12_backend_t::get_sampler_gpu_index(gfx_sampler_handle id)
	{
		const sampler_t& s = _samplers.get(id);
		return _descriptors.get(s.descriptor_index).index;
	}

	gfx_shader_handle dx12_backend_t::create_shader(const shader_desc_t& desc, const vector_t<shader_blob_t>& blobs, gfx_bind_layout_handle existing_layout, span_t<u8> layout_data)
	{
		SFG_VERIFY_RENDER_NOT_RUNNING_OR_RENDER_THREAD();

		const gfx_shader_handle id = _shaders.add();
		shader_t&				sh = _shaders.get(id);
		sh.topology				   = static_cast<u8>(get_topology(desc.topo));

		if (layout_data.size != 0)
		{
			throw_if_failed(_device->CreateRootSignature(0, layout_data.data, layout_data.size, IID_PPV_ARGS(&sh.root_signature)));
			sh.owns_root_sig = true;
		}
		else
			sh.root_signature = _bind_layouts.get(existing_layout).root_signature;

		/* Early out if compute */
		const auto it = vector_util::find_if(blobs, [](const shader_blob_t& b) -> bool { return b.stage == shader_stage::compute; });
		if (it != blobs.end())
		{
			D3D12_COMPUTE_PIPELINE_STATE_DESC cpsd = {};
			cpsd.pRootSignature					   = sh.root_signature.Get();
			cpsd.CS								   = {it->data.data, it->data.size};
			cpsd.NodeMask						   = 0;
			_device->CreateComputePipelineState(&cpsd, IID_PPV_ARGS(&sh.ptr));
			NAME_DX12_OBJECT_CSTR(sh.ptr, desc.debug_name.c_str());
			return id;
		}

		vector_t<D3D12_INPUT_ELEMENT_DESC> input_layout;

		for (size_t i = 0; i < desc.inputs.size(); i++)
		{
			const vertex_input_t& inp = desc.inputs[i];
			input_layout.push_back({
				.SemanticName		  = inp.name.c_str(),
				.SemanticIndex		  = inp.index,
				.Format				  = get_format(inp.format),
				.InputSlot			  = inp.location,
				.AlignedByteOffset	  = static_cast<u32>(inp.offset),
				.InputSlotClass		  = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0,
			});
		}

		D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {
			.pRootSignature	 = sh.root_signature.Get(),
			.BlendState		 = {},
			.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
			.InputLayout	 = {input_layout.data(), static_cast<UINT>(input_layout.size())},
		};

		pso_desc.BlendState						   = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		pso_desc.BlendState.AlphaToCoverageEnable  = desc.flags.is_set(shader_flags::shf_enable_alpha_to_cov);
		pso_desc.BlendState.IndependentBlendEnable = false;

		const u32 attachment_count = static_cast<u32>(desc.attachments.size());

		for (u32 i = 0; i < attachment_count; i++)
		{
			const shader_color_attachment_t& att					  = desc.attachments[i];
			pso_desc.BlendState.RenderTarget[i].BlendEnable			  = att.blend_attachment.blend_enabled;
			pso_desc.BlendState.RenderTarget[i].SrcBlend			  = get_blend(att.blend_attachment.src_color_blend_factor);
			pso_desc.BlendState.RenderTarget[i].DestBlend			  = get_blend(att.blend_attachment.dst_color_blend_factor);
			pso_desc.BlendState.RenderTarget[i].SrcBlendAlpha		  = get_blend(att.blend_attachment.src_alpha_blend_factor);
			pso_desc.BlendState.RenderTarget[i].DestBlendAlpha		  = get_blend(att.blend_attachment.dst_alpha_blend_factor);
			pso_desc.BlendState.RenderTarget[i].BlendOp				  = get_blend_op(att.blend_attachment.color_blend_op);
			pso_desc.BlendState.RenderTarget[i].BlendOpAlpha		  = get_blend_op(att.blend_attachment.alpha_blend_op);
			pso_desc.BlendState.RenderTarget[i].LogicOpEnable		  = desc.flags.is_set(shader_flags::shf_enable_blend_logic_op);
			pso_desc.BlendState.RenderTarget[i].LogicOp				  = get_logic_op(desc.blend_logic_op);
			pso_desc.BlendState.RenderTarget[i].RenderTargetWriteMask = get_color_mask(att.blend_attachment.color_comp_flags);
			pso_desc.RTVFormats[i]									  = get_format(att.format);
		}

		pso_desc.DepthStencilState.DepthEnable		   = desc.depth_stencil_desc.flags.is_set(depth_stencil_flags::dsf_depth_test);
		pso_desc.DepthStencilState.DepthWriteMask	   = desc.depth_stencil_desc.flags.is_set(depth_stencil_flags::dsf_depth_write) ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
		pso_desc.DepthStencilState.DepthFunc		   = get_compare_op(desc.depth_stencil_desc.depth_compare);
		pso_desc.DepthStencilState.StencilEnable	   = desc.depth_stencil_desc.flags.is_set(depth_stencil_flags::dsf_enable_stencil);
		pso_desc.DepthStencilState.StencilReadMask	   = desc.depth_stencil_desc.stencil_compare_mask;
		pso_desc.DepthStencilState.StencilWriteMask	   = desc.depth_stencil_desc.stencil_write_mask;
		pso_desc.DepthStencilState.FrontFace		   = {get_stencil_op(desc.depth_stencil_desc.front_stencil_state.fail_op),
														  get_stencil_op(desc.depth_stencil_desc.front_stencil_state.depth_fail_op),
														  get_stencil_op(desc.depth_stencil_desc.front_stencil_state.pass_op),
														  get_compare_op(desc.depth_stencil_desc.front_stencil_state.compare_op)};
		pso_desc.DepthStencilState.BackFace			   = {get_stencil_op(desc.depth_stencil_desc.front_stencil_state.fail_op),
														  get_stencil_op(desc.depth_stencil_desc.front_stencil_state.depth_fail_op),
														  get_stencil_op(desc.depth_stencil_desc.front_stencil_state.pass_op),
														  get_compare_op(desc.depth_stencil_desc.front_stencil_state.compare_op)};
		pso_desc.SampleMask							   = UINT_MAX;
		pso_desc.SampleDesc.Count					   = desc.samples;
		pso_desc.PrimitiveTopologyType				   = get_topology_type(desc.topo);
		pso_desc.NumRenderTargets					   = attachment_count;
		pso_desc.RasterizerState.FillMode			   = get_fill_mode(desc.fill);
		pso_desc.RasterizerState.CullMode			   = get_cull_mode(desc.cull);
		pso_desc.RasterizerState.FrontCounterClockwise = desc.front == front_face::ccw;
		pso_desc.RasterizerState.DepthBias			   = static_cast<u32>(desc.depth_bias_constant);
		pso_desc.RasterizerState.DepthBiasClamp		   = desc.depth_bias_clamp;
		pso_desc.RasterizerState.SlopeScaledDepthBias  = desc.depth_bias_slope;
		pso_desc.DSVFormat							   = get_format(desc.depth_stencil_desc.attachment_format);

		for (const shader_blob_t& bl : blobs)
		{
			const void*	 byte_code = (void*)bl.data.data;
			const SIZE_T length	   = static_cast<SIZE_T>(bl.data.size);

			if (bl.stage == shader_stage::vertex)
			{
				pso_desc.VS.pShaderBytecode = byte_code;
				pso_desc.VS.BytecodeLength	= length;
			}
			else if (bl.stage == shader_stage::fragment)
			{
				pso_desc.PS.pShaderBytecode = byte_code;
				pso_desc.PS.BytecodeLength	= length;
			}
			else
			{
				SFG_ASSERT(false);
			}
		}

		throw_if_failed(_device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&sh.ptr)));
		NAME_DX12_OBJECT_CSTR(sh.ptr, desc.debug_name.c_str());
		return id;
	}

	void dx12_backend_t::destroy_shader(gfx_shader_handle id)
	{
		SFG_VERIFY_RENDER_NOT_RUNNING_OR_RENDER_THREAD();

		shader_t& sh = _shaders.get(id);

		if (sh.owns_root_sig)
			sh.root_signature.Reset();
		sh.ptr.Reset();
		_shaders.remove(id);
	}

	gfx_bind_group_handle dx12_backend_t::create_empty_bind_group()
	{
		SFG_VERIFY_RENDER_NOT_RUNNING_OR_RENDER_THREAD();

		const gfx_bind_group_handle id = _bind_groups.add();
		return id;
	}

	void dx12_backend_t::bind_group_add_descriptor(gfx_bind_group_handle group, u8 root_param_index, u8 type)
	{
		SFG_VERIFY_RENDER_NOT_RUNNING_OR_RENDER_THREAD();

		bind_group_t& bind_group_t = _bind_groups.get(group);
		bind_group_t.bindings.push_back({});
		group_binding_t& gbinding = bind_group_t.bindings.back();
		gbinding.descriptor_index = _descriptors.add();
		gbinding.root_param_index = root_param_index;
		gbinding.count			  = 1;
		gbinding.binding_type	  = type;

		const binding_type tp = static_cast<binding_type>(type);
		SFG_ASSERT(tp != binding_type::pointer && tp != binding_type::sampler_t);
	}

	void dx12_backend_t::bind_group_add_constant(gfx_bind_group_handle group, u8 root_param_index, u8* data, u8 count)
	{
		SFG_VERIFY_RENDER_NOT_RUNNING_OR_RENDER_THREAD();

		bind_group_t& bind_group_t = _bind_groups.get(group);
		bind_group_t.bindings.push_back({});
		group_binding_t& gbinding = bind_group_t.bindings.back();
		gbinding.descriptor_index = _descriptors.add();
		gbinding.root_param_index = root_param_index;
		gbinding.count			  = count;
		gbinding.binding_type	  = binding_type::constant;
		gbinding.constants		  = data;
	}

	void dx12_backend_t::bind_group_add_pointer(gfx_bind_group_handle group, u8 root_param_index, u8 count, bool is_sampler)
	{
		SFG_VERIFY_RENDER_NOT_RUNNING_OR_RENDER_THREAD();

		bind_group_t& bind_group_t = _bind_groups.get(group);
		bind_group_t.bindings.push_back({});
		group_binding_t& gbinding = bind_group_t.bindings.back();
		gbinding.descriptor_index = _descriptors.add();
		gbinding.root_param_index = root_param_index;
		gbinding.count			  = count;

		if (is_sampler)
		{
			descriptor_handle_t& dh = _descriptors.get(gbinding.descriptor_index);
			dh						= _heap_gpu_sampler.get_heap_handle_block(count);
			gbinding.binding_type	= binding_type::sampler_t;
		}
		else
		{
			descriptor_handle_t& dh = _descriptors.get(gbinding.descriptor_index);
			dh						= _heap_gpu_buffer.get_heap_handle_block(count);
			gbinding.binding_type	= binding_type::pointer;
		}
	}

	void dx12_backend_t::bind_group_update_constants(gfx_bind_group_handle id, u8 binding_index, u8* constants, u8 count)
	{
		SFG_VERIFY_RENDER_NOT_RUNNING_OR_RENDER_THREAD();

		bind_group_t&	 group	   = _bind_groups.get(id);
		group_binding_t& binding_t = group.bindings[binding_index];
		binding_t.constants		   = constants;
		binding_t.count			   = count;
	}

	void dx12_backend_t::bind_group_update_descriptor(gfx_bind_group_handle id, u8 binding_index, gfx_resource_handle res_id)
	{
		SFG_VERIFY_RENDER_NOT_RUNNING_OR_RENDER_THREAD();

		bind_group_t&	 group	   = _bind_groups.get(id);
		group_binding_t& binding_t = group.bindings[binding_index];

		descriptor_handle_t& dh	 = _descriptors.get(binding_t.descriptor_index);
		const resource_t&	 res = _resources.get(res_id);
		dh.gpu					 = res.ptr->GetResource()->GetGPUVirtualAddress();
	}

	void dx12_backend_t::bind_group_update_pointer(gfx_bind_group_handle id, u8 binding_index, const bind_group_pointer_t* updates, u16 update_count)
	{
		SFG_VERIFY_RENDER_NOT_RUNNING_OR_RENDER_THREAD();

		bind_group_t&	 group	   = _bind_groups.get(id);
		group_binding_t& binding_t = group.bindings[binding_index];

		_reuse_dest_descriptors_buffer.resize(0);
		_reuse_dest_descriptors_sampler.resize(0);
		_reuse_src_descriptors_buffer.resize(0);
		_reuse_src_descriptors_sampler.resize(0);

		descriptor_handle_t& binding_dh = _descriptors.get(binding_t.descriptor_index);

		for (u8 i = 0; i < update_count; i++)
		{
			const bind_group_pointer_t& p = updates[i];
			if (p.type == binding_type::texture_binding)
			{
				const texture_t& txt = _textures.get(p.texture_t);
				SFG_ASSERT(txt.view_count > p.view);
				const texture_view_t& view = txt.views[p.view];
				SFG_ASSERT(view.type == static_cast<u8>(view_type::sampled));

				const descriptor_handle_t& dh = _descriptors.get(view.handle);
				_reuse_src_descriptors_buffer.push_back({dh.cpu});
				_reuse_dest_descriptors_buffer.push_back({binding_dh.cpu + p.pointer_index * _heap_gpu_buffer.get_descriptor_size()});
			}
			else if (p.type == binding_type::sampler_t)
			{
				const sampler_t&		   smp = _samplers.get(p.sampler_t);
				const descriptor_handle_t& dh  = _descriptors.get(smp.descriptor_index);
				_reuse_src_descriptors_sampler.push_back({dh.cpu});
				_reuse_dest_descriptors_sampler.push_back({binding_dh.cpu + p.pointer_index * _heap_gpu_sampler.get_descriptor_size()});
			}
			else if (p.type == binding_type::ubo || p.type == binding_type::ssbo || p.type == binding_type::uav)
			{
				const resource_t&	 res = _resources.get(p.resource_t);
				descriptor_handle_t& dh	 = _descriptors.get(res.descriptor_index);
				_reuse_src_descriptors_buffer.push_back({dh.cpu});
				_reuse_dest_descriptors_buffer.push_back({binding_dh.cpu + p.pointer_index * _heap_gpu_buffer.get_descriptor_size()});
			}
			else
			{
				SFG_ASSERT(false);
			}
		}

		const u32 descriptor_count_buffer  = static_cast<u32>(_reuse_dest_descriptors_buffer.size());
		const u32 descriptor_count_sampler = static_cast<u32>(_reuse_dest_descriptors_sampler.size());

		if (descriptor_count_buffer != 0)
			_device->CopyDescriptors(descriptor_count_buffer, _reuse_dest_descriptors_buffer.data(), NULL, descriptor_count_buffer, _reuse_src_descriptors_buffer.data(), NULL, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		if (descriptor_count_sampler != 0)
			_device->CopyDescriptors(descriptor_count_sampler, _reuse_dest_descriptors_sampler.data(), NULL, descriptor_count_sampler, _reuse_src_descriptors_sampler.data(), NULL, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	}

	void dx12_backend_t::bind_group_update_pointer(gfx_bind_group_handle group, u8 binding_index, const vector_t<bind_group_pointer_t>& updates)
	{
		bind_group_update_pointer(group, binding_index, updates.data(), static_cast<u16>(updates.size()));
	}

	void dx12_backend_t::destroy_bind_group(gfx_bind_group_handle id)
	{
		SFG_VERIFY_RENDER_NOT_RUNNING_OR_RENDER_THREAD();

		bind_group_t& group = _bind_groups.get(id);

		const u8 sz = static_cast<u8>(group.bindings.size());
		for (u8 i = 0; i < sz; i++)
		{
			const group_binding_t& binding_t = group.bindings[i];

			const binding_type type = static_cast<binding_type>(binding_t.binding_type);

			if (type == binding_type::sampler_t)
			{
				const descriptor_handle_t& dh = _descriptors.get(binding_t.descriptor_index);
				_heap_gpu_sampler.remove_handle(dh);
			}
			else if (type == binding_type::pointer)
			{
				const descriptor_handle_t& dh = _descriptors.get(binding_t.descriptor_index);
				_heap_gpu_buffer.remove_handle(dh);
			}

			_descriptors.remove(binding_t.descriptor_index);
		}

		_bind_groups.remove(id);
	}

	gfx_command_buffer_handle dx12_backend_t::create_command_buffer(const command_buffer_desc_t& desc)
	{
		SFG_VERIFY_RENDER_NOT_RUNNING_OR_RENDER_THREAD();

		const gfx_command_buffer_handle	   id		= _command_buffers.add();
		const gfx_command_allocator_handle alloc_id = create_command_allocator(static_cast<u8>(desc.type));
		command_allocator_t&			   alloc	= _command_allocators.get(alloc_id);
		command_buffer_t&				   cmd		= _command_buffers.get(id);
		throw_if_failed(_device->CreateCommandList(0, get_command_type(desc.type), alloc.ptr.Get(), nullptr, IID_PPV_ARGS(cmd.ptr.GetAddressOf())));
		cmd.allocator	= alloc_id;
		cmd.is_transfer = desc.type == command_type::transfer;
		cmd.ptr->Close();
		NAME_DX12_OBJECT_CSTR(cmd.ptr, desc.debug_name);
		return id;
	}

	void dx12_backend_t::destroy_command_buffer(gfx_command_buffer_handle id)
	{
		SFG_VERIFY_RENDER_NOT_RUNNING_OR_RENDER_THREAD();

		command_buffer_t& cmd = _command_buffers.get(id);
		cmd.ptr.Reset();
		destroy_command_allocator(cmd.allocator);
		_command_buffers.remove(id);
	}

	gfx_command_allocator_handle dx12_backend_t::create_command_allocator(u8 type)
	{
		SFG_VERIFY_RENDER_NOT_RUNNING_OR_RENDER_THREAD();

		const gfx_command_allocator_handle id	 = _command_allocators.add();
		command_allocator_t&			   alloc = _command_allocators.get(id);
		throw_if_failed(_device->CreateCommandAllocator(get_command_type(static_cast<command_type>(type)), IID_PPV_ARGS(alloc.ptr.GetAddressOf())));
		return id;
	}

	void dx12_backend_t::destroy_command_allocator(gfx_command_allocator_handle id)
	{
		SFG_VERIFY_RENDER_NOT_RUNNING_OR_RENDER_THREAD();

		command_allocator_t& alloc = _command_allocators.get(id);
		alloc.ptr.Reset();
		_command_allocators.remove(id);
	}

	gfx_queue_handle dx12_backend_t::create_queue(const queue_desc_t& desc)
	{
		SFG_VERIFY_RENDER_NOT_RUNNING_OR_RENDER_THREAD();

		const gfx_queue_handle id = _queues.add();
		queue_t&			   q  = _queues.get(id);

		const D3D12_COMMAND_QUEUE_DESC queue_desc_t = {
			.Type  = get_command_type(desc.type),
			.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
		};
		throw_if_failed(_device->CreateCommandQueue(&queue_desc_t, IID_PPV_ARGS(&q.ptr)));
		NAME_DX12_OBJECT_CSTR(q.ptr, desc.debug_name);
		return id;
	}

	gfx_bind_layout_handle dx12_backend_t::create_empty_bind_layout()
	{
		SFG_VERIFY_RENDER_NOT_RUNNING_OR_RENDER_THREAD();

		const gfx_bind_layout_handle id = _bind_layouts.add();
		_reuse_root_params.resize(0);
		_reuse_static_samplers.resize(0);
		_reuse_root_ranges.resize(0);
		return id;
	}

	gfx_indirect_signature_handle dx12_backend_t::create_draw_indirect_signature(gfx_bind_layout_handle bind_layout_id, size_t struct_size)
	{
		const gfx_indirect_signature_handle id = _indirect_signatures.add();
		indirect_signature_t&				s  = _indirect_signatures.get(id);

		D3D12_INDIRECT_ARGUMENT_DESC argumentDesc = {};
		argumentDesc.Type						  = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;

		D3D12_COMMAND_SIGNATURE_DESC commandSignatureDesc = {};
		commandSignatureDesc.pArgumentDescs				  = &argumentDesc;
		commandSignatureDesc.NumArgumentDescs			  = 1;
		commandSignatureDesc.ByteStride					  = struct_size;

		ID3D12RootSignature* root = NULL;

		if (!bind_layout_id.is_null())
		{
			bind_layout_t& bl = _bind_layouts.get(bind_layout_id);
			root			  = bl.root_signature.Get();
		}

		try
		{
			_device->CreateCommandSignature(&commandSignatureDesc, root, IID_PPV_ARGS(&s.signature));
		}
		catch (HrException e)
		{
			SFG_ERR("Error while creating indirect signature {0}", e.what());
		}
		return id;
	}

	gfx_indirect_signature_handle dx12_backend_t::create_dispatch_indirect_signature(gfx_bind_layout_handle bind_layout_id, size_t struct_size)
	{
		const gfx_indirect_signature_handle id = _indirect_signatures.add();
		indirect_signature_t&				s  = _indirect_signatures.get(id);

		D3D12_INDIRECT_ARGUMENT_DESC argumentDesc = {};
		argumentDesc.Type						  = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;

		D3D12_COMMAND_SIGNATURE_DESC commandSignatureDesc = {};
		commandSignatureDesc.pArgumentDescs				  = &argumentDesc;
		commandSignatureDesc.NumArgumentDescs			  = 1;
		commandSignatureDesc.ByteStride					  = struct_size;

		ID3D12RootSignature* root = NULL;

		if (!bind_layout_id.is_null())
		{
			bind_layout_t& bl = _bind_layouts.get(bind_layout_id);
			root			  = bl.root_signature.Get();
		}

		try
		{
			_device->CreateCommandSignature(&commandSignatureDesc, root, IID_PPV_ARGS(&s.signature));
		}
		catch (HrException e)
		{
			SFG_ERR("Error while creating indirect signature {0}", e.what());
		}
		return id;
	}

	void dx12_backend_t::destroy_indirect_signature(gfx_indirect_signature_handle sig)
	{
		indirect_signature_t& s = _indirect_signatures.get(sig);
		s.signature.Reset();
		_indirect_signatures.remove(sig);
	}

	void dx12_backend_t::bind_layout_add_constant(gfx_bind_layout_handle layout, u32 count, u32 set, u32 binding_t, u8 vis)
	{
		SFG_VERIFY_RENDER_NOT_RUNNING_OR_RENDER_THREAD();

		const D3D12_SHADER_VISIBILITY visibility = get_visibility(static_cast<shader_stage>(vis));
		_reuse_root_params.push_back({});
		CD3DX12_ROOT_PARAMETER1& param = _reuse_root_params.back();
		param.InitAsConstants(count, binding_t, set, visibility);
	}

	void dx12_backend_t::bind_layout_add_descriptor(gfx_bind_layout_handle layout, u8 type, u32 set, u32 binding_t, u8 vis)
	{
		SFG_VERIFY_RENDER_NOT_RUNNING_OR_RENDER_THREAD();

		const D3D12_SHADER_VISIBILITY visibility = get_visibility(static_cast<shader_stage>(vis));
		_reuse_root_params.push_back({});
		CD3DX12_ROOT_PARAMETER1& param = _reuse_root_params.back();

		const binding_type tp = static_cast<binding_type>(type);

		if (tp == binding_type::ubo)
			param.InitAsConstantBufferView(binding_t, set, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, visibility);
		else if (tp == binding_type::ssbo)
			param.InitAsShaderResourceView(binding_t, set, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, visibility);
		else if (tp == binding_type::uav)
			param.InitAsUnorderedAccessView(binding_t, set, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, visibility);
		else
			SFG_ASSERT(false);
	}

	void dx12_backend_t::bind_layout_add_pointer(gfx_bind_layout_handle layout, const vector_t<bind_layout_pointer_param_t>& pointer_params, u8 vis)
	{
		SFG_VERIFY_RENDER_NOT_RUNNING_OR_RENDER_THREAD();

		const u32 start = static_cast<u32>(_reuse_root_ranges.size());

		UINT offset = 0;

		for (const bind_layout_pointer_param_t& p : pointer_params)
		{
			_reuse_root_ranges.push_back({});
			CD3DX12_DESCRIPTOR_RANGE1& range = _reuse_root_ranges.back();

			const D3D12_DESCRIPTOR_RANGE_FLAGS flags = p.is_volatile ? D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE : D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

			if (p.type == binding_type::ubo)
			{
				range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, p.count, p.binding_t, p.set, flags, offset);
			}
			else if (p.type == binding_type::ssbo || p.type == binding_type::texture_binding)
			{
				range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, p.count, p.binding_t, p.set, flags, offset);
			}
			else if (p.type == binding_type::uav)
			{
				range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, p.count, p.binding_t, p.set, flags, offset);
			}
			else if (p.type == binding_type::sampler_t)
			{
				range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, p.count, p.binding_t, p.set, D3D12_DESCRIPTOR_RANGE_FLAG_NONE, offset);
			}

			offset += p.count;
		}

		const u32					  size_now	 = static_cast<u32>(_reuse_root_ranges.size());
		const D3D12_SHADER_VISIBILITY visibility = get_visibility(static_cast<shader_stage>(vis));
		_reuse_root_params.push_back({});
		CD3DX12_ROOT_PARAMETER1& param = _reuse_root_params.back();
		param.InitAsDescriptorTable(size_now - start, &_reuse_root_ranges[start], visibility);
	}

	void dx12_backend_t::bind_layout_add_immutable_sampler(gfx_bind_layout_handle layout, u32 set, u32 binding_t, const sampler_desc_t& desc, u8 vis)
	{
		SFG_VERIFY_RENDER_NOT_RUNNING_OR_RENDER_THREAD();

		const D3D12_SHADER_VISIBILITY visibility = get_visibility(static_cast<shader_stage>(vis));

		_reuse_static_samplers.push_back({
			.Filter			  = get_filter(desc.flags),
			.AddressU		  = get_address_mode(desc.address_u),
			.AddressV		  = get_address_mode(desc.address_v),
			.AddressW		  = get_address_mode(desc.address_w),
			.MipLODBias		  = static_cast<FLOAT>(desc.lod_bias),
			.MaxAnisotropy	  = desc.anisotropy,
			.ComparisonFunc	  = desc.flags.is_set(sampler_flags::saf_compare) ? get_compare_op(desc.compare) : D3D12_COMPARISON_FUNC_NONE,
			.MinLOD			  = desc.min_lod,
			.MaxLOD			  = desc.max_lod,
			.ShaderRegister	  = binding_t,
			.RegisterSpace	  = set,
			.ShaderVisibility = visibility,
		});
	}

	void dx12_backend_t::finalize_bind_layout(gfx_bind_layout_handle id, bool is_compute, bool is_dyn_indexed, const char* name)
	{
		SFG_VERIFY_RENDER_NOT_RUNNING_OR_RENDER_THREAD();

		D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

		if (is_dyn_indexed)
		{
			flags |= D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;
			flags |= D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;
		}

		if (!is_compute)
		{
			flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		}

		bind_layout_t&								layout = _bind_layouts.get(id);
		const CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSig(static_cast<u32>(_reuse_root_params.size()), _reuse_root_params.data(), static_cast<u32>(_reuse_static_samplers.size()), _reuse_static_samplers.data(), flags);
		ComPtr<ID3DBlob>							signature = nullptr;
		ComPtr<ID3DBlob>							error	  = nullptr;
		const HRESULT								res		  = D3D12SerializeVersionedRootSignature(&rootSig, &signature, &error);

		if (FAILED(res) && error)
		{
			const char* error_msg = static_cast<const char*>(error->GetBufferPointer());
			SFG_ERR("{0}", error_msg);
			throw_if_failed(res);
		}

		throw_if_failed(_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&layout.root_signature)));
		NAME_DX12_OBJECT_CSTR(layout.root_signature, name);
		signature.Reset();
		error.Reset();
		_reuse_root_params.resize(0);
		_reuse_static_samplers.resize(0);
		_reuse_root_ranges.resize(0);
	}

	void dx12_backend_t::destroy_bind_layout(gfx_bind_layout_handle id)
	{
		SFG_VERIFY_RENDER_NOT_RUNNING_OR_RENDER_THREAD();

		bind_layout_t& layout = _bind_layouts.get(id);
		layout.root_signature.Reset();
		_bind_layouts.remove(id);
	}

	void dx12_backend_t::destroy_queue(gfx_queue_handle id)
	{
		SFG_VERIFY_RENDER_NOT_RUNNING_OR_RENDER_THREAD();

		queue_t& q = _queues.get(id);
		q.ptr.Reset();
		_queues.remove(id);
	}

	void dx12_backend_t::wait_for_fence(ID3D12Fence* fence, u64 value) const
	{
		const UINT64 last_fence = fence->GetCompletedValue();

		if (last_fence < value)
		{
			try
			{
				HANDLE event_handle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
				if (event_handle == nullptr)
				{
					throw_if_failed(HRESULT_FROM_WIN32(GetLastError()));
				}
				else
				{
					throw_if_failed(fence->SetEventOnCompletion(value, event_handle));
					WaitForSingleObject(event_handle, INFINITE);
					CloseHandle(event_handle);
				}
			}
			catch (HrException e)
			{
				SFG_ERR("Error while waiting for fence! {0}", e.what());
			}
		}
	}

	void dx12_backend_t::cmd_begin_render_pass(gfx_command_buffer_handle cmd_id, const command_begin_render_pass_t& cmd)
	{
		command_buffer_t&			buffer_t = _command_buffers.get(cmd_id);
		ID3D12GraphicsCommandList4* cmd_list = buffer_t.ptr.Get();

		static_vector_t<D3D12_RENDER_PASS_RENDER_TARGET_DESC, 8> color_attachments;
		color_attachments.resize(cmd.color_attachment_count);

		for (u32 i = 0; i < cmd.color_attachment_count; i++)
		{
			const render_pass_color_attachment_t& att = cmd.color_attachments[i];
			const texture_t&					  txt = _textures.get(att.texture_t);

			const texture_view_t& view = txt.views[att.view_index];
			SFG_ASSERT(view.type == static_cast<u8>(view_type::render_target));
			const descriptor_handle_t& dh = _descriptors.get(view.handle);

			CD3DX12_CLEAR_VALUE cv;
			cv.Format	= static_cast<DXGI_FORMAT>(txt.format);
			cv.Color[0] = att.clear_color.x;
			cv.Color[1] = att.clear_color.y;
			cv.Color[2] = att.clear_color.z;
			cv.Color[3] = att.clear_color.w;
			const D3D12_RENDER_PASS_BEGINNING_ACCESS color_begin{get_load_op(att.load_op), {cv}};
			const D3D12_RENDER_PASS_ENDING_ACCESS	 color_end{get_store_op(att.store_op), {}};

			color_attachments[i] = {dh.cpu, color_begin, color_end};
		}

		cmd_list->BeginRenderPass(cmd.color_attachment_count, color_attachments.data(), NULL, D3D12_RENDER_PASS_FLAG_NONE);
	}

	void dx12_backend_t::cmd_begin_render_pass_depth(gfx_command_buffer_handle cmd_id, const command_begin_render_pass_depth_t& cmd)
	{
		command_buffer_t&										 buffer_t = _command_buffers.get(cmd_id);
		ID3D12GraphicsCommandList4*								 cmd_list = buffer_t.ptr.Get();
		static_vector_t<D3D12_RENDER_PASS_RENDER_TARGET_DESC, 8> color_attachments;
		color_attachments.resize(cmd.color_attachment_count);

		for (u32 i = 0; i < cmd.color_attachment_count; i++)
		{
			const render_pass_color_attachment_t& att = cmd.color_attachments[i];
			const texture_t&					  txt = _textures.get(att.texture_t);

			const texture_view_t& view = txt.views[att.view_index];
			SFG_ASSERT(view.type == static_cast<u8>(view_type::render_target));
			const descriptor_handle_t& dh = _descriptors.get(view.handle);

			CD3DX12_CLEAR_VALUE cv;
			cv.Format	= static_cast<DXGI_FORMAT>(txt.format);
			cv.Color[0] = att.clear_color.x;
			cv.Color[1] = att.clear_color.y;
			cv.Color[2] = att.clear_color.z;
			cv.Color[3] = att.clear_color.w;
			const D3D12_RENDER_PASS_BEGINNING_ACCESS color_begin{get_load_op(att.load_op), {cv}};
			const D3D12_RENDER_PASS_ENDING_ACCESS	 color_end{get_store_op(att.store_op), {}};

			color_attachments[i] = {dh.cpu, color_begin, color_end};
		}

		const texture_t& depth_txt = _textures.get(cmd.depth_stencil_attachment.texture_t);

		const texture_view_t& depth_view = depth_txt.views[cmd.depth_stencil_attachment.view_index];
		SFG_ASSERT(depth_view.type == static_cast<u8>(view_type::depth_stencil));
		const descriptor_handle_t& depth_dh = _descriptors.get(depth_view.handle);

		const CD3DX12_CLEAR_VALUE				   clear_depth_stencil{static_cast<DXGI_FORMAT>(depth_txt.format), cmd.depth_stencil_attachment.clear_depth, cmd.depth_stencil_attachment.clear_stencil};
		const D3D12_RENDER_PASS_BEGINNING_ACCESS   depth_begin{get_load_op(cmd.depth_stencil_attachment.depth_load_op), {clear_depth_stencil}};
		const D3D12_RENDER_PASS_BEGINNING_ACCESS   stencil_begin{get_load_op(cmd.depth_stencil_attachment.stencil_load_op), {clear_depth_stencil}};
		const D3D12_RENDER_PASS_ENDING_ACCESS	   depth_end{get_store_op(cmd.depth_stencil_attachment.depth_store_op), {}};
		const D3D12_RENDER_PASS_ENDING_ACCESS	   stencil_end{get_store_op(cmd.depth_stencil_attachment.stencil_store_op), {}};
		const D3D12_RENDER_PASS_DEPTH_STENCIL_DESC depth_stencil_desc{depth_dh.cpu, depth_begin, stencil_begin, depth_end, stencil_end};
		cmd_list->BeginRenderPass(cmd.color_attachment_count, color_attachments.data(), &depth_stencil_desc, D3D12_RENDER_PASS_FLAG_NONE);
	}

	void dx12_backend_t::cmd_begin_render_pass_depth_read_only(gfx_command_buffer_handle cmd_id, const command_begin_render_pass_depth_t& cmd)
	{
		command_buffer_t&										 buffer_t = _command_buffers.get(cmd_id);
		ID3D12GraphicsCommandList4*								 cmd_list = buffer_t.ptr.Get();
		static_vector_t<D3D12_RENDER_PASS_RENDER_TARGET_DESC, 8> color_attachments;
		color_attachments.resize(cmd.color_attachment_count);

		for (u32 i = 0; i < cmd.color_attachment_count; i++)
		{
			const render_pass_color_attachment_t& att = cmd.color_attachments[i];
			const texture_t&					  txt = _textures.get(att.texture_t);
			const descriptor_handle_t&			  dh  = _descriptors.get(txt.views[att.view_index].handle);

			CD3DX12_CLEAR_VALUE cv;
			cv.Format	= static_cast<DXGI_FORMAT>(txt.format);
			cv.Color[0] = att.clear_color.x;
			cv.Color[1] = att.clear_color.y;
			cv.Color[2] = att.clear_color.z;
			cv.Color[3] = att.clear_color.w;
			const D3D12_RENDER_PASS_BEGINNING_ACCESS color_begin{get_load_op(att.load_op), {cv}};
			const D3D12_RENDER_PASS_ENDING_ACCESS	 color_end{get_store_op(att.store_op), {}};

			color_attachments[i] = {dh.cpu, color_begin, color_end};
		}

		D3D12_RENDER_PASS_FLAGS flags	   = D3D12_RENDER_PASS_FLAG_NONE;
		const texture_t&		depth_txt  = _textures.get(cmd.depth_stencil_attachment.texture_t);
		const texture_view_t&	depth_view = depth_txt.views[cmd.depth_stencil_attachment.view_index];
		SFG_ASSERT(depth_view.type == static_cast<u8>(view_type::depth_stencil));
		const descriptor_handle_t& depth_dh = _descriptors.get(depth_view.handle);

		const CD3DX12_CLEAR_VALUE				   clear_depth_stencil{static_cast<DXGI_FORMAT>(depth_txt.format), cmd.depth_stencil_attachment.clear_depth, cmd.depth_stencil_attachment.clear_stencil};
		const D3D12_RENDER_PASS_BEGINNING_ACCESS   depth_begin{get_load_op(cmd.depth_stencil_attachment.depth_load_op), {clear_depth_stencil}};
		const D3D12_RENDER_PASS_BEGINNING_ACCESS   stencil_begin{get_load_op(cmd.depth_stencil_attachment.stencil_load_op), {clear_depth_stencil}};
		const D3D12_RENDER_PASS_ENDING_ACCESS	   depth_end{get_store_op(cmd.depth_stencil_attachment.depth_store_op), {}};
		const D3D12_RENDER_PASS_ENDING_ACCESS	   stencil_end{get_store_op(cmd.depth_stencil_attachment.stencil_store_op), {}};
		const D3D12_RENDER_PASS_DEPTH_STENCIL_DESC depth_stencil_desc{depth_dh.cpu, depth_begin, stencil_begin, depth_end, stencil_end};
		cmd_list->BeginRenderPass(cmd.color_attachment_count, color_attachments.data(), &depth_stencil_desc, D3D12_RENDER_PASS_FLAG_BIND_READ_ONLY_DEPTH);
	}

	void dx12_backend_t::cmd_begin_render_pass_depth_only(gfx_command_buffer_handle cmd_id, const command_begin_render_pass_depth_only_t& cmd)
	{
		command_buffer_t&			buffer_t = _command_buffers.get(cmd_id);
		ID3D12GraphicsCommandList4* cmd_list = buffer_t.ptr.Get();

		const texture_t&	  depth_txt	 = _textures.get(cmd.depth_stencil_attachment.texture_t);
		const texture_view_t& depth_view = depth_txt.views[cmd.depth_stencil_attachment.view_index];
		SFG_ASSERT(depth_view.type == static_cast<u8>(view_type::depth_stencil));
		const descriptor_handle_t& depth_dh = _descriptors.get(depth_view.handle);

		const CD3DX12_CLEAR_VALUE				   clear_depth_stencil{static_cast<DXGI_FORMAT>(depth_txt.format), cmd.depth_stencil_attachment.clear_depth, cmd.depth_stencil_attachment.clear_stencil};
		const D3D12_RENDER_PASS_BEGINNING_ACCESS   depth_begin{get_load_op(cmd.depth_stencil_attachment.depth_load_op), {clear_depth_stencil}};
		const D3D12_RENDER_PASS_BEGINNING_ACCESS   stencil_begin{get_load_op(cmd.depth_stencil_attachment.stencil_load_op), {clear_depth_stencil}};
		const D3D12_RENDER_PASS_ENDING_ACCESS	   depth_end{get_store_op(cmd.depth_stencil_attachment.depth_store_op), {}};
		const D3D12_RENDER_PASS_ENDING_ACCESS	   stencil_end{get_store_op(cmd.depth_stencil_attachment.stencil_store_op), {}};
		const D3D12_RENDER_PASS_DEPTH_STENCIL_DESC depth_stencil_desc{depth_dh.cpu, depth_begin, stencil_begin, depth_end, stencil_end};
		cmd_list->BeginRenderPass(0, NULL, &depth_stencil_desc, D3D12_RENDER_PASS_FLAG_NONE);
	}

	void dx12_backend_t::cmd_begin_render_pass_swapchain(gfx_command_buffer_handle cmd_id, const command_begin_render_pass_swapchain_t& cmd)
	{
		command_buffer_t&										 buffer_t = _command_buffers.get(cmd_id);
		ID3D12GraphicsCommandList4*								 cmd_list = buffer_t.ptr.Get();
		static_vector_t<D3D12_RENDER_PASS_RENDER_TARGET_DESC, 8> color_attachments;

		color_attachments.resize(cmd.color_attachment_count);

		for (u32 i = 0; i < cmd.color_attachment_count; i++)
		{
			const render_pass_swapchain_attachment_t& att = cmd.color_attachments[i];
			const swapchain_t&						  swp = _swapchains.get(att.swapchain_t);
			const descriptor_handle_t&				  dh  = _descriptors.get(swp.rtv_indices[swp.image_index]);

			CD3DX12_CLEAR_VALUE cv;
			cv.Format	= static_cast<DXGI_FORMAT>(swp.format);
			cv.Color[0] = att.clear_color.x;
			cv.Color[1] = att.clear_color.y;
			cv.Color[2] = att.clear_color.z;
			cv.Color[3] = att.clear_color.w;
			const D3D12_RENDER_PASS_BEGINNING_ACCESS color_begin{get_load_op(att.load_op), {cv}};
			const D3D12_RENDER_PASS_ENDING_ACCESS	 color_end{get_store_op(att.store_op), {}};

			color_attachments[i] = {dh.cpu, color_begin, color_end};
		}

		cmd_list->BeginRenderPass(cmd.color_attachment_count, color_attachments.data(), NULL, D3D12_RENDER_PASS_FLAG_NONE);
	}

	void dx12_backend_t::cmd_begin_render_pass_swapchain_depth(gfx_command_buffer_handle cmd_id, const command_begin_render_pass_swapchain_depth_t& cmd)
	{
		command_buffer_t&										 buffer_t = _command_buffers.get(cmd_id);
		ID3D12GraphicsCommandList4*								 cmd_list = buffer_t.ptr.Get();
		static_vector_t<D3D12_RENDER_PASS_RENDER_TARGET_DESC, 8> color_attachments;

		color_attachments.resize(cmd.color_attachment_count);

		for (u32 i = 0; i < cmd.color_attachment_count; i++)
		{
			const render_pass_swapchain_attachment_t& att = cmd.color_attachments[i];
			const swapchain_t&						  swp = _swapchains.get(att.swapchain_t);
			const descriptor_handle_t&				  dh  = _descriptors.get(swp.rtv_indices[swp.image_index]);

			CD3DX12_CLEAR_VALUE cv;
			cv.Format	= static_cast<DXGI_FORMAT>(swp.format);
			cv.Color[0] = att.clear_color.x;
			cv.Color[1] = att.clear_color.y;
			cv.Color[2] = att.clear_color.z;
			cv.Color[3] = att.clear_color.w;
			const D3D12_RENDER_PASS_BEGINNING_ACCESS color_begin{get_load_op(att.load_op), {cv}};
			const D3D12_RENDER_PASS_ENDING_ACCESS	 color_end{get_store_op(att.store_op), {}};

			color_attachments[i] = {dh.cpu, color_begin, color_end};
		}

		const texture_t&	  depth_txt	 = _textures.get(cmd.depth_stencil_attachment.texture_t);
		const texture_view_t& depth_view = depth_txt.views[cmd.depth_stencil_attachment.view_index];
		SFG_ASSERT(depth_view.type == static_cast<u8>(view_type::depth_stencil));
		const descriptor_handle_t& depth_dh = _descriptors.get(depth_view.handle);

		const CD3DX12_CLEAR_VALUE				   clear_depth_stencil{static_cast<DXGI_FORMAT>(depth_txt.format), cmd.depth_stencil_attachment.clear_depth, cmd.depth_stencil_attachment.clear_stencil};
		const D3D12_RENDER_PASS_BEGINNING_ACCESS   depth_begin{get_load_op(cmd.depth_stencil_attachment.depth_load_op), {clear_depth_stencil}};
		const D3D12_RENDER_PASS_BEGINNING_ACCESS   stencil_begin{get_load_op(cmd.depth_stencil_attachment.stencil_load_op), {clear_depth_stencil}};
		const D3D12_RENDER_PASS_ENDING_ACCESS	   depth_end{get_store_op(cmd.depth_stencil_attachment.depth_store_op), {}};
		const D3D12_RENDER_PASS_ENDING_ACCESS	   stencil_end{get_store_op(cmd.depth_stencil_attachment.stencil_store_op), {}};
		const D3D12_RENDER_PASS_DEPTH_STENCIL_DESC depth_stencil_desc{depth_dh.cpu, depth_begin, stencil_begin, depth_end, stencil_end};
		cmd_list->BeginRenderPass(cmd.color_attachment_count, color_attachments.data(), &depth_stencil_desc, D3D12_RENDER_PASS_FLAG_NONE);
	}

	void dx12_backend_t::cmd_end_render_pass(gfx_command_buffer_handle cmd_id, const command_end_render_pass_t& cmd) const
	{
		const command_buffer_t&		buffer_t = _command_buffers.get(cmd_id);
		ID3D12GraphicsCommandList4* cmd_list = buffer_t.ptr.Get();
		cmd_list->EndRenderPass();
	}

	void dx12_backend_t::cmd_set_scissors(gfx_command_buffer_handle cmd_id, const command_set_scissors_t& cmd) const
	{
		const command_buffer_t&		buffer_t = _command_buffers.get(cmd_id);
		ID3D12GraphicsCommandList4* cmd_list = buffer_t.ptr.Get();
		D3D12_RECT					sc;
		sc.left	  = static_cast<LONG>(cmd.x);
		sc.top	  = static_cast<LONG>(cmd.y);
		sc.right  = static_cast<LONG>(cmd.x + cmd.width);
		sc.bottom = static_cast<LONG>(cmd.y + cmd.height);
		cmd_list->RSSetScissorRects(1, &sc);
	}

	void dx12_backend_t::cmd_set_viewport(gfx_command_buffer_handle cmd_id, const command_set_viewport_t& cmd) const
	{
		const command_buffer_t&		buffer_t = _command_buffers.get(cmd_id);
		ID3D12GraphicsCommandList4* cmd_list = buffer_t.ptr.Get();
		D3D12_VIEWPORT				vp;
		vp.MinDepth = cmd.min_depth;
		vp.MaxDepth = cmd.max_depth;
		vp.Height	= static_cast<float>(cmd.height);
		vp.Width	= static_cast<float>(cmd.width);
		vp.TopLeftX = cmd.x;
		vp.TopLeftY = cmd.y;
		cmd_list->RSSetViewports(1, &vp);
	}

	void dx12_backend_t::cmd_bind_pipeline(gfx_command_buffer_handle cmd_id, const command_bind_pipeline_t& cmd) const
	{
		const command_buffer_t&		buffer_t = _command_buffers.get(cmd_id);
		ID3D12GraphicsCommandList4* cmd_list = buffer_t.ptr.Get();
		const shader_t&				sh		 = _shaders.get(cmd.pipeline);

		cmd_list->SetPipelineState(sh.ptr.Get());
		cmd_list->IASetPrimitiveTopology(static_cast<D3D12_PRIMITIVE_TOPOLOGY>(sh.topology));
	}

	void dx12_backend_t::cmd_bind_pipeline_compute(gfx_command_buffer_handle cmd_id, const command_bind_pipeline_compute_t& cmd) const
	{
		const command_buffer_t&		buffer_t = _command_buffers.get(cmd_id);
		ID3D12GraphicsCommandList4* cmd_list = buffer_t.ptr.Get();
		const shader_t&				sh		 = _shaders.get(cmd.pipeline);
		cmd_list->SetPipelineState(sh.ptr.Get());
	}

	void dx12_backend_t::cmd_draw_instanced(gfx_command_buffer_handle cmd_id, const command_draw_instanced_t& cmd) const
	{
		const command_buffer_t&		buffer_t = _command_buffers.get(cmd_id);
		ID3D12GraphicsCommandList4* cmd_list = buffer_t.ptr.Get();
		cmd_list->DrawInstanced(cmd.vertex_count_per_instance, cmd.instance_count, cmd.start_vertex_location, cmd.start_instance_location);
	}

	void dx12_backend_t::cmd_draw_indexed_instanced(gfx_command_buffer_handle cmd_id, const command_draw_indexed_instanced_t& cmd) const
	{
		const command_buffer_t&		buffer_t = _command_buffers.get(cmd_id);
		ID3D12GraphicsCommandList4* cmd_list = buffer_t.ptr.Get();
		cmd_list->DrawIndexedInstanced(cmd.index_count_per_instance, cmd.instance_count, cmd.start_index_location, cmd.base_vertex_location, cmd.start_instance_location);
	}

	void dx12_backend_t::cmd_execute_indirect(gfx_command_buffer_handle cmd_id, const command_draw_indirect_t& cmd) const
	{
		const command_buffer_t&		buffer_t = _command_buffers.get(cmd_id);
		ID3D12GraphicsCommandList4* cmd_list = buffer_t.ptr.Get();

		cmd_list->ExecuteIndirect(_indirect_signatures.get(cmd.indirect_signature_t).signature.Get(), cmd.count, _resources.get(cmd.indirect_buffer).ptr->GetResource(), cmd.indirect_buffer_offset, NULL, 0);
	}

	void dx12_backend_t::cmd_bind_vertex_buffers(gfx_command_buffer_handle cmd_id, const command_bind_vertex_buffers_t& cmd) const
	{
		const command_buffer_t&		   buffer_t = _command_buffers.get(cmd_id);
		ID3D12GraphicsCommandList4*	   cmd_list = buffer_t.ptr.Get();
		const resource_t&			   res		= _resources.get(cmd.buffer_t);
		const D3D12_VERTEX_BUFFER_VIEW view		= {
				.BufferLocation = res.ptr->GetResource()->GetGPUVirtualAddress(),
				.SizeInBytes	= static_cast<u32>(res.size),
				.StrideInBytes	= static_cast<u32>(cmd.vertex_size),
		};

		cmd_list->IASetVertexBuffers(cmd.slot, 1, &view);
	}

	void dx12_backend_t::cmd_bind_index_buffers(gfx_command_buffer_handle cmd_id, const command_bind_index_buffers_t& cmd) const
	{
		const command_buffer_t&		buffer_t = _command_buffers.get(cmd_id);
		ID3D12GraphicsCommandList4* cmd_list = buffer_t.ptr.Get();
		const resource_t&			res		 = _resources.get(cmd.buffer_t);

		const D3D12_INDEX_BUFFER_VIEW view = {
			.BufferLocation = res.ptr->GetResource()->GetGPUVirtualAddress(),
			.SizeInBytes	= static_cast<u32>(res.size),
			.Format			= cmd.index_size == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT,
		};

		cmd_list->IASetIndexBuffer(&view);
	}

	void dx12_backend_t::cmd_copy_resource(gfx_command_buffer_handle cmd_id, const command_copy_resource_t& cmd) const
	{
		const command_buffer_t&		buffer_t = _command_buffers.get(cmd_id);
		ID3D12GraphicsCommandList4* cmd_list = buffer_t.ptr.Get();
		const resource_t&			src_res	 = _resources.get(cmd.source);
		const resource_t&			dest_res = _resources.get(cmd.destination);
		cmd_list->CopyResource(dest_res.ptr->GetResource(), src_res.ptr->GetResource());
	}

	void dx12_backend_t::cmd_copy_resource_region(gfx_command_buffer_handle cmd_id, const command_copy_resource_region_t& cmd) const
	{
		const command_buffer_t&		buffer_t = _command_buffers.get(cmd_id);
		ID3D12GraphicsCommandList4* cmd_list = buffer_t.ptr.Get();
		const resource_t&			src_res	 = _resources.get(cmd.source);
		const resource_t&			dest_res = _resources.get(cmd.destination);
		cmd_list->CopyBufferRegion(dest_res.ptr->GetResource(), cmd.dst_offset, src_res.ptr->GetResource(), cmd.src_offset, cmd.size);
	}

	u32 dx12_backend_t::get_texture_size(u32 width, u32 height, u32 bpp) const
	{
		const u32 row_pitch	  = static_cast<u32>((width * bpp + (D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1)) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1));
		const u32 slice_pitch = row_pitch * height;
		return slice_pitch;
	}

	u32 dx12_backend_t::align_texture_size(u32 size) const
	{
		return (size + D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT - 1) & ~(D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT - 1);
	}

	u32 dx12_backend_t::align_texture_size_pitch(u32 size) const
	{
		return (size + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1);
	}

	void* dx12_backend_t::adjust_buffer_pitch(void* data, u32 width, u32 height, u8 bpp, u32& out_total_size) const
	{
		const u32 _bpp		= static_cast<u32>(bpp);
		const u32 alignment = D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
		const u32 row_pitch = (width * _bpp + (alignment - 1)) & ~(alignment - 1);
		out_total_size		= row_pitch * height;
		char* buffer_t		= reinterpret_cast<char*>(new u8[out_total_size]);
		char* src			= reinterpret_cast<char*>(data);
		char* dst			= buffer_t;

		if (dst != 0)
		{
			for (u32 i = 0; i < height; ++i)
			{
				SFG_MEMCPY(dst, src, width * _bpp);
				dst += row_pitch;
				src += width * _bpp;
			}
		}

		return buffer_t;
	}

	void dx12_backend_t::cmd_begin_event(gfx_command_buffer_handle cmd_id, const char* label)
	{
		command_buffer_t&			buffer_t = _command_buffers.get(cmd_id);
		ID3D12GraphicsCommandList4* cmd_list = buffer_t.ptr.Get();
#ifdef SFG_DEBUG
		PIXBeginEvent(cmd_list, PIX_COLOR(120, 255, 100), "%s", label);
#endif
	}

	void dx12_backend_t::cmd_end_event(gfx_command_buffer_handle cmd_id)
	{
		command_buffer_t&			buffer_t = _command_buffers.get(cmd_id);
		ID3D12GraphicsCommandList4* cmd_list = buffer_t.ptr.Get();
#ifdef SFG_DEBUG
		PIXEndEvent(cmd_list);
#endif
	}

	void dx12_backend_t::cmd_copy_buffer_to_texture(gfx_command_buffer_handle cmd_id, const command_copy_buffer_to_texture_t& cmd)
	{
		command_buffer_t&			buffer_t = _command_buffers.get(cmd_id);
		ID3D12GraphicsCommandList4* cmd_list = buffer_t.ptr.Get();

		static_vector_t<D3D12_SUBRESOURCE_DATA, 16> subresource_data;

		for (u8 i = 0; i < cmd.mip_levels; i++)
		{
			const texture_buffer_t& tb		  = cmd.textures[i];
			const LONG_PTR			row_pitch = tb.size.x * tb.bpp;
			// static_cast<LONG_PTR>((tb.size.x * tb.bpp + (D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1)) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1));

			const D3D12_SUBRESOURCE_DATA texture_data = {
				.pData		= tb.pixels,
				.RowPitch	= row_pitch,
				.SlicePitch = row_pitch * static_cast<LONG_PTR>(tb.size.y),
			};

			subresource_data.push_back(texture_data);
		}

		resource_t& res = _resources.get(cmd.intermediate_buffer);
		texture_t&	txt = _textures.get(cmd.destination_texture);
		UpdateSubresources(cmd_list, txt.ptr->GetResource(), res.ptr->GetResource(), 0, cmd.mip_levels * cmd.destination_slice, cmd.mip_levels, subresource_data.data());
	}

	void dx12_backend_t::cmd_copy_texture_to_buffer(gfx_command_buffer_handle cmd_id, const command_copy_texture_to_buffer_t& cmd) const
	{
		const command_buffer_t&		buffer_t  = _command_buffers.get(cmd_id);
		ID3D12GraphicsCommandList4* cmd_list  = buffer_t.ptr.Get();
		const texture_t&			txt		  = _textures.get(cmd.src_texture);
		const resource_t&			res		  = _resources.get(cmd.dest_buffer);
		const u32					row_pitch = static_cast<UINT>((cmd.size.x * cmd.bpp + (D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1)) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1));

		const D3D12_TEXTURE_COPY_LOCATION dest_location = {
			.pResource = res.ptr->GetResource(),
			.Type	   = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
			.PlacedFootprint =
				{
					.Offset = 0,
					.Footprint =
						{
							.Format	  = static_cast<DXGI_FORMAT>(txt.format),
							.Width	  = static_cast<UINT>(cmd.size.x),
							.Height	  = static_cast<UINT>(cmd.size.y),
							.Depth	  = 1,
							.RowPitch = row_pitch,
						},
				},
		};

		const D3D12_TEXTURE_COPY_LOCATION src_location = {
			.pResource		  = txt.ptr->GetResource(),
			.Type			  = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
			.SubresourceIndex = D3D12CalcSubresource(cmd.src_mip, cmd.src_layer, 0, 1, 1),
		};

		cmd_list->CopyTextureRegion(&dest_location, 0, 0, 0, &src_location, NULL);
	}

	void dx12_backend_t::cmd_copy_texture_to_texture(gfx_command_buffer_handle cmd_id, const command_copy_texture_to_texture_t& cmd) const
	{
		const command_buffer_t&		buffer_t = _command_buffers.get(cmd_id);
		ID3D12GraphicsCommandList4* cmd_list = buffer_t.ptr.Get();
		const texture_t&			src		 = _textures.get(cmd.source);
		const texture_t&			dst		 = _textures.get(cmd.destination);

		const D3D12_TEXTURE_COPY_LOCATION src_location = {
			.pResource		  = src.ptr->GetResource(),
			.Type			  = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
			.SubresourceIndex = static_cast<UINT>(cmd.source_layer * cmd.source_total_mips + cmd.source_mip),
		};

		const D3D12_TEXTURE_COPY_LOCATION dst_location = {
			.pResource		  = dst.ptr->GetResource(),
			.Type			  = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
			.SubresourceIndex = static_cast<UINT>(cmd.destination_layer * cmd.destination_total_mips + cmd.destination_mip),
		};

		cmd_list->CopyTextureRegion(&dst_location, 0, 0, 0, &src_location, nullptr);
	}

	void dx12_backend_t::cmd_bind_constants(gfx_command_buffer_handle cmd_id, const command_bind_constants_t& cmd) const
	{
		const command_buffer_t&		buffer_t = _command_buffers.get(cmd_id);
		ID3D12GraphicsCommandList4* cmd_list = buffer_t.ptr.Get();
		cmd_list->SetGraphicsRoot32BitConstants(static_cast<u32>(cmd.param_index), static_cast<u32>(cmd.count), cmd.data, cmd.offset);
	}

	void dx12_backend_t::cmd_bind_constants_compute(gfx_command_buffer_handle cmd_id, const command_bind_constants_t& cmd) const
	{
		const command_buffer_t&		buffer_t = _command_buffers.get(cmd_id);
		ID3D12GraphicsCommandList4* cmd_list = buffer_t.ptr.Get();
		cmd_list->SetComputeRoot32BitConstants(static_cast<u32>(cmd.param_index), static_cast<u32>(cmd.count), cmd.data, cmd.offset);
	}

	void dx12_backend_t::cmd_bind_layout(gfx_command_buffer_handle cmd_id, const command_bind_layout_t& cmd) const
	{
		const command_buffer_t&		buffer_t = _command_buffers.get(cmd_id);
		ID3D12GraphicsCommandList4* cmd_list = buffer_t.ptr.Get();
		const bind_layout_t&		layout	 = _bind_layouts.get(cmd.layout);
		cmd_list->SetGraphicsRootSignature(layout.root_signature.Get());
	}

	void dx12_backend_t::cmd_bind_layout_compute(gfx_command_buffer_handle cmd_id, const command_bind_layout_compute_t& cmd) const
	{
		const command_buffer_t&		buffer_t = _command_buffers.get(cmd_id);
		ID3D12GraphicsCommandList4* cmd_list = buffer_t.ptr.Get();
		const bind_layout_t&		layout	 = _bind_layouts.get(cmd.layout);
		cmd_list->SetComputeRootSignature(layout.root_signature.Get());
	}

	void dx12_backend_t::cmd_bind_group(gfx_command_buffer_handle cmd_id, const command_bind_group_t& cmd) const
	{
		const command_buffer_t&		buffer_t = _command_buffers.get(cmd_id);
		ID3D12GraphicsCommandList4* cmd_list = buffer_t.ptr.Get();
		const bind_group_t&			group	 = _bind_groups.get(cmd.group);
		const u8					sz		 = static_cast<u8>(group.bindings.size());

		for (u8 i = 0; i < sz; i++)
		{
			const group_binding_t&	   binding_t = group.bindings[i];
			const descriptor_handle_t& dh		 = _descriptors.get(binding_t.descriptor_index);
			const binding_type		   type		 = static_cast<binding_type>(binding_t.binding_type);

			if (type == binding_type::sampler_t || type == binding_type::pointer)
				cmd_list->SetGraphicsRootDescriptorTable(binding_t.root_param_index, {dh.gpu});
			else if (type == binding_type::ubo)
				cmd_list->SetGraphicsRootConstantBufferView(binding_t.root_param_index, dh.gpu);
			else if (type == binding_type::ssbo)
				cmd_list->SetGraphicsRootShaderResourceView(binding_t.root_param_index, dh.gpu);
			else if (type == binding_type::uav)
				cmd_list->SetGraphicsRootUnorderedAccessView(binding_t.root_param_index, dh.gpu);
			else if (type == binding_type::constant)
				cmd_list->SetGraphicsRoot32BitConstants(binding_t.root_param_index, binding_t.count, binding_t.constants, 0);
		}
	}

	void dx12_backend_t::cmd_bind_group_compute(gfx_command_buffer_handle cmd_id, const command_bind_group_t& cmd) const
	{
		const command_buffer_t&		buffer_t = _command_buffers.get(cmd_id);
		ID3D12GraphicsCommandList4* cmd_list = buffer_t.ptr.Get();
		const bind_group_t&			group	 = _bind_groups.get(cmd.group);
		const u8					sz		 = static_cast<u8>(group.bindings.size());

		for (u8 i = 0; i < sz; i++)
		{
			const group_binding_t&	   binding_t = group.bindings[i];
			const descriptor_handle_t& dh		 = _descriptors.get(binding_t.descriptor_index);
			const binding_type		   type		 = static_cast<binding_type>(binding_t.binding_type);

			if (type == binding_type::sampler_t || type == binding_type::pointer)
				cmd_list->SetComputeRootDescriptorTable(binding_t.root_param_index, {dh.gpu});
			else if (type == binding_type::ubo)
				cmd_list->SetComputeRootConstantBufferView(binding_t.root_param_index, dh.gpu);
			else if (type == binding_type::ssbo)
				cmd_list->SetComputeRootShaderResourceView(binding_t.root_param_index, dh.gpu);
			else if (type == binding_type::uav)
				cmd_list->SetComputeRootUnorderedAccessView(binding_t.root_param_index, dh.gpu);
			else if (type == binding_type::constant)
				cmd_list->SetComputeRoot32BitConstants(binding_t.root_param_index, binding_t.count, binding_t.constants, 0);
		}
	}

	void dx12_backend_t::cmd_dispatch(gfx_command_buffer_handle cmd_id, const command_dispatch_t& cmd) const
	{
		const command_buffer_t&		buffer_t = _command_buffers.get(cmd_id);
		ID3D12GraphicsCommandList4* cmd_list = buffer_t.ptr.Get();
		cmd_list->Dispatch(cmd.group_size_x, cmd.group_size_y, cmd.group_size_z);
	}

	void dx12_backend_t::cmd_barrier(gfx_command_buffer_handle cmd_id, const command_barrier_t& cmd)
	{
		command_buffer_t&			buffer_t = _command_buffers.get(cmd_id);
		ID3D12GraphicsCommandList4* cmd_list = buffer_t.ptr.Get();

		static_vector_t<CD3DX12_RESOURCE_BARRIER, 256> barriers;

		for (u16 i = 0; i < cmd.barrier_count; i++)
		{
			const barrier_t& barrier_t = cmd.barriers[i];

			ID3D12Resource* res = nullptr;

			if (barrier_t.flags.is_set(barrier_flags::baf_is_resource))
			{
				res = _resources.get(barrier_t.resource_t).ptr->GetResource();
			}
			else if (barrier_t.flags.is_set(barrier_flags::baf_is_swapchain))
			{
				swapchain_t& swp = _swapchains.get(barrier_t.swapchain_t);
				res				 = swp.textures[swp.image_index].Get();
			}
			else
			{
				texture_t& txt = _textures.get(barrier_t.texture_t);
				res			   = txt.ptr->GetResource();
			}

			if (barrier_t.flags.is_set(barrier_flags::baf_is_uav))
				barriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(res));
			else
				barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(res, get_resource_state(barrier_t.from_states), get_resource_state(barrier_t.to_states)));
		}

		if (!barriers.empty())
			cmd_list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
	}

}
