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

#include "editor_renderer.hpp"
#include "data/vector_util.hpp"
#include "app/engine_resources.hpp"

// math
#include "math/vector2.hpp"
#include "math/vector4.hpp"
#include "math/math.hpp"

// gfx
#include "gfx/backend/backend.hpp"
#include "gfx/common/descriptions.hpp"
#include "gfx/common/commands.hpp"
#include "gfx/util/gfx_util.hpp"
#include "gfx/texture_queue.hpp"
#include "gfx/common/render_target_definitions.hpp"

#include "platform/window.hpp"

#include "gui/vekt.hpp"

#include <tracy/Tracy.hpp>
namespace SFG
{
	void editor_renderer::init(window& window, texture_queue* texture_queue, size_t vtx_sz, size_t idx_sz)
	{
		_gfx_data.texture_queue = texture_queue;
		_gfx_data.screen_size	= window.get_size();

		create_textures(_gfx_data.screen_size);
		// Shaders
		_shaders.gui_default = engine_resources::get().get_shader_direct(engine_resource_ident::shader_gui_default).get_hw();
		_shaders.gui_sdf	 = engine_resources::get().get_shader_direct(engine_resource_ident::shader_gui_sdf).get_hw();
		_shaders.gui_text	 = engine_resources::get().get_shader_direct(engine_resource_ident::shader_gui_text).get_hw();
		_shaders.gui_texture = engine_resources::get().get_shader_direct(engine_resource_ident::shader_gui_texture).get_hw();

		engine_resources::get().add_shader_reload_listener([this](engine_resource_ident type, shader_direct& sh) {
			if (type == engine_resource_ident::shader_gui_default)
			{
				_shaders.gui_default = sh.get_hw();
				return;
			}
			if (type == engine_resource_ident::shader_gui_text)
			{
				_shaders.gui_text = sh.get_hw();
				return;
			}
			if (type == engine_resource_ident::shader_gui_sdf)
			{
				_shaders.gui_sdf = sh.get_hw();
				return;
			}

			if (type == engine_resource_ident::shader_gui_texture)
			{
				_shaders.gui_texture = sh.get_hw();
				return;
			}
		});

		gfx_backend* backend = gfx_backend::get();
		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			pfd.buf_pass_data.create({.size = sizeof(gui_pass_view), .flags = resource_flags::rf_cpu_visible | resource_flags::rf_constant_buffer, .debug_name = "cbv_editor_gui_pass"});
			pfd.buf_gui_vtx.create({.size = static_cast<u32>(vtx_sz), .flags = resource_flags::rf_vertex_buffer | resource_flags::rf_cpu_visible, .debug_name = "editor_gui_vertex_stg"},
								   {.size = static_cast<u32>(vtx_sz), .flags = resource_flags::rf_vertex_buffer | resource_flags::rf_gpu_only, .debug_name = "editor_gui_vertex_gpu"});
			pfd.buf_gui_idx.create({.size = static_cast<u32>(idx_sz), .flags = resource_flags::rf_index_buffer | resource_flags::rf_cpu_visible, .debug_name = "editor_gui_index_stg"},
								   {.size = static_cast<u32>(idx_sz), .flags = resource_flags::rf_index_buffer | resource_flags::rf_gpu_only, .debug_name = "editor_gui_index_gpu"});
		}

		_snapshots = new vekt::snapshot[SNAPSHOTS_SIZE];
		for (u32 i = 0; i < SNAPSHOTS_SIZE; i++)
			_snapshots[i].init(vtx_sz, idx_sz);
	}

	void editor_renderer::uninit()
	{

		gfx_backend* backend = gfx_backend::get();

		for (auto& ref : _gfx_data.atlases)
		{
			backend->destroy_texture(ref.texture);
			backend->destroy_resource(ref.intermediate_buffer);
		}
		_gfx_data.atlases.clear();

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];
			pfd.buf_pass_data.destroy();
			pfd.buf_gui_vtx.destroy();
			pfd.buf_gui_idx.destroy();
		}

		destroy_textures();

		// Release snapshots
		for (u32 i = 0; i < SNAPSHOTS_SIZE; i++)
			_snapshots[i].uninit();
		delete[] _snapshots;
		_snapshots = nullptr;
	}

	void editor_renderer::draw_end(vekt::builder* builder)
	{

		const vekt::vector<vekt::draw_buffer>& draw_buffers = builder->get_draw_buffers();

		const u8 published = _snapshot_write;
		_snapshots[published].copy(draw_buffers);

		_snapshot_latest.store((i8)published, std::memory_order_release);
		const i8 in_use = _snapshot_in_use.load(std::memory_order_acquire);
		u8		 next	= (published + 1) % SNAPSHOTS_SIZE;
		if ((i8)next == in_use)
			next = (next + 1) % SNAPSHOTS_SIZE;

		_snapshot_write = next;
	}

	void editor_renderer::prepare(proxy_manager& pm, gfx_id cmd_buffer, u8 frame_index)
	{
		ZoneScoped;

		gfx_backend* backend = gfx_backend::get();

		per_frame_data& pfd = _pfd[frame_index];
		pfd.reset();

		// -----------------------------------------------------------------------------
		// render pass data
		// -----------------------------------------------------------------------------

		const gui_pass_view view = {
			.proj		   = matrix4x4::ortho_reverse_z(0, static_cast<f32>(_gfx_data.screen_size.x), 0, static_cast<f32>(_gfx_data.screen_size.y), 0.0f, 1.0f),
			.sdf_thickness = 0.5f,
			.sdf_softness  = 0.05f,
		};
		pfd.buf_pass_data.buffer_data(0, (void*)&view, sizeof(gui_pass_view));

		// -----------------------------------------------------------------------------
		// flush commands and draw gui here.
		// -----------------------------------------------------------------------------

		const i8 idx = _snapshot_latest.load(std::memory_order_acquire);
		if (idx >= 0)
		{
			_snapshot_in_use.store(idx, std::memory_order_release);

			const vekt::vector<vekt::draw_buffer>& draw_buffers = _snapshots[idx].draw_buffers;
			for (const vekt::draw_buffer& db : draw_buffers)
				draw_vekt(frame_index, db);
			_snapshot_in_use.store(-1, std::memory_order_release);
		}

		// -----------------------------------------------------------------------------
		// copy vtx-index
		// -----------------------------------------------------------------------------
		{
			static_vector<barrier, 2> barriers;
			barriers.push_back({
				.from_states = resource_state::resource_state_index_buffer,
				.to_states	 = resource_state::resource_state_copy_dest,
				.resource	 = pfd.buf_gui_idx.get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
			});

			barriers.push_back({
				.from_states = resource_state::resource_state_vertex_cbv,
				.to_states	 = resource_state::resource_state_copy_dest,
				.resource	 = pfd.buf_gui_vtx.get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
			});
			backend->cmd_barrier(cmd_buffer, {.barriers = barriers.data(), .barrier_count = 2});
			barriers.resize(0);

			// Copy from staging to GPU
			if (pfd.counter_vtx != 0)
				pfd.buf_gui_vtx.copy_region(cmd_buffer, 0, pfd.counter_vtx * sizeof(vekt::vertex));
			if (pfd.counter_idx != 0)
				pfd.buf_gui_idx.copy_region(cmd_buffer, 0, pfd.counter_idx * sizeof(vekt::index));

			barriers.push_back({
				.from_states = resource_state::resource_state_copy_dest,
				.to_states	 = resource_state::resource_state_index_buffer,
				.resource	 = pfd.buf_gui_idx.get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
			});

			barriers.push_back({
				.from_states = resource_state::resource_state_copy_dest,
				.to_states	 = resource_state::resource_state_vertex_cbv,
				.resource	 = pfd.buf_gui_vtx.get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
			});

			backend->cmd_barrier(cmd_buffer, {.barriers = barriers.data(), .barrier_count = 2});
		}
	}

	void editor_renderer::render(const render_params& p)
	{
		ZoneScoped;

		gfx_backend*	backend = gfx_backend::get();
		per_frame_data& pfd		= _pfd[p.frame_index];

		const gfx_id cmd_buffer			 = p.cmd_buffer;
		const gfx_id render_target		 = pfd.hw_rt;
		const gfx_id gui_vertex			 = pfd.buf_gui_vtx.get_gpu();
		const gfx_id gui_index			 = pfd.buf_gui_idx.get_gpu();
		const u16	 dc_count			 = pfd.draw_call_count;
		const u32	 gpu_index_pass_data = pfd.buf_pass_data.get_index();

		static_vector<barrier, 1> barriers;

		// in barrier
		{
			barriers.push_back({
				.from_states = resource_state::resource_state_ps_resource | resource_state::resource_state_non_ps_resource,
				.to_states	 = resource_state::resource_state_render_target,
				.resource	 = render_target,
				.flags		 = barrier_flags::baf_is_texture,
			});

			backend->cmd_barrier(cmd_buffer,
								 {
									 .barriers		= barriers.data(),
									 .barrier_count = static_cast<u16>(barriers.size()),
								 });
		}

		BEGIN_DEBUG_EVENT(backend, cmd_buffer, "editor_pass");

		const render_pass_color_attachment att = {
			.clear_color = vector4(0, 0, 0, 0.0f),
			.texture	 = render_target,
			.load_op	 = load_op::clear,
			.store_op	 = store_op::store,
			.view_index	 = 0,
		};

		backend->cmd_begin_render_pass(cmd_buffer,
									   {
										   .color_attachments	   = &att,
										   .color_attachment_count = 1,
									   });

		backend->cmd_bind_layout(cmd_buffer, {.layout = p.global_layout});
		backend->cmd_bind_group(cmd_buffer, {.group = p.global_group});

		static_vector<u32, 4> rp_constants;
		rp_constants.push_back(gpu_index_pass_data);

		// Bind GUI pass constants (CBV index)
		backend->cmd_bind_constants(cmd_buffer, {.data = (u8*)rp_constants.data(), .offset = constant_index_rp_constant0, .count = static_cast<u8>(rp_constants.size()), .param_index = rpi_constants});

		backend->cmd_set_scissors(cmd_buffer, {.width = static_cast<u16>(p.size.x), .height = static_cast<u16>(p.size.y)});
		backend->cmd_set_viewport(cmd_buffer,
								  {
									  .min_depth = 0.0f,
									  .max_depth = 1.0f,
									  .width	 = static_cast<u16>(p.size.x),
									  .height	 = static_cast<u16>(p.size.y),

								  });

		// Vertex/index buffers
		backend->cmd_bind_vertex_buffers(cmd_buffer, {.buffer = gui_vertex, .vertex_size = sizeof(vekt::vertex)});
		backend->cmd_bind_index_buffers(cmd_buffer, {.buffer = gui_index, .index_size = sizeof(vekt::index)});

		gfx_id last_pipeline	   = NULL_GFX_ID;
		u32	   last_atlas_constant = UINT32_MAX;

		for (u16 i = 0; i < dc_count; i++)
		{
			gui_draw_call& dc = _gui_draw_calls[i];

			backend->cmd_set_scissors(cmd_buffer, {.x = dc.scissors.x, .y = dc.scissors.y, .width = dc.scissors.z, .height = dc.scissors.w});

			if (last_pipeline == NULL_GFX_ID || last_pipeline != dc.shader)
			{
				backend->cmd_bind_pipeline(cmd_buffer, {.pipeline = dc.shader});
				last_pipeline = dc.shader;
			}

			if (last_atlas_constant == UINT32_MAX || (dc.atlas_gpu_index != last_atlas_constant))
			{
				backend->cmd_bind_constants(cmd_buffer, {.data = (u8*)&dc.atlas_gpu_index, .offset = constant_index_mat_constant2, .count = 1, .param_index = rpi_constants});
				last_atlas_constant = dc.atlas_gpu_index;
			}

			gpu_index bd_texture = NULL_GPU_INDEX;

			if (dc.ud_type == editor_gui_user_data_type::world_rt)
				bd_texture = p.world_rt_index;
			else if (dc.ud_type == editor_gui_user_data_type::colors_rt)
				bd_texture = p.color_rt_index;
			else if (dc.ud_type == editor_gui_user_data_type::normals_rt)
				bd_texture = p.normals_rt_index;
			else if (dc.ud_type == editor_gui_user_data_type::orm_rt)
				bd_texture = p.orm_rt_index;
			else if (dc.ud_type == editor_gui_user_data_type::emissive_rt)
				bd_texture = p.emissive_rt_index;
			else if (dc.ud_type == editor_gui_user_data_type::depth_rt)
				bd_texture = p.depth_rt_index;
			else if (dc.ud_type == editor_gui_user_data_type::lighting_rt)
				bd_texture = p.lighting_rt_index;
			else if (dc.ud_type == editor_gui_user_data_type::bloom_rt)
				bd_texture = p.bloom_rt_index;
			else if (dc.ud_type == editor_gui_user_data_type::ssao_rt)
				bd_texture = p.ssao_rt_index;

			if (bd_texture != NULL_GPU_INDEX)
				backend->cmd_bind_constants(cmd_buffer, {.data = (u8*)&bd_texture, .offset = constant_index_mat_constant1, .count = 1, .param_index = rpi_constants});

			backend->cmd_draw_indexed_instanced(cmd_buffer,
												{
													.index_count_per_instance = dc.index_count,
													.instance_count			  = 1,
													.start_index_location	  = dc.start_idx,
													.base_vertex_location	  = dc.start_vtx,
													.start_instance_location  = 0,
												});
		}

		backend->cmd_end_render_pass(cmd_buffer, {});
		END_DEBUG_EVENT(backend, cmd_buffer);

		// out barrier
		{
			barriers.resize(0);
			barriers.push_back({
				.from_states = resource_state::resource_state_render_target,
				.to_states	 = resource_state::resource_state_ps_resource | resource_state::resource_state_non_ps_resource,
				.resource	 = render_target,
				.flags		 = barrier_flags::baf_is_texture,
			});

			backend->cmd_barrier(cmd_buffer,
								 {
									 .barriers		= barriers.data(),
									 .barrier_count = static_cast<u16>(barriers.size()),
								 });
		}
	}

	void editor_renderer::resize(const vector2ui16& size)
	{
		if (_gfx_data.screen_size == size)
			return;

		_gfx_data.screen_size = size;

		destroy_textures();
		create_textures(size);
	}

	gpu_index editor_renderer::get_atlas_gpu_index(vekt::atlas* atl)
	{
		for (const atlas_ref& r : _gfx_data.atlases)
		{
			if (r.atlas == atl)
				return r.texture_gpu_index;
		}

		return NULL_GPU_INDEX;
	}

	void editor_renderer::create_textures(const vector2ui16& size)
	{
		gfx_backend* backend = gfx_backend::get();

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			pfd.hw_rt = backend->create_texture({
				.texture_format = render_target_definitions::get_format_editor(),
				.size			= size,
				.flags			= texture_flags::tf_render_target | texture_flags::tf_is_2d | texture_flags::tf_sampled,
				.views			= {{.type = view_type::render_target}, {.type = view_type::sampled}},
				.clear_values	= {0.0f, 0.0f, 0.0f, 0.0f},
				.debug_name		= "editor_rt",
			});

			pfd.gpu_index_rt = backend->get_texture_gpu_index(pfd.hw_rt, 1);
		}
	}

	void editor_renderer::destroy_textures()
	{
		gfx_backend* backend = gfx_backend::get();

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			backend->destroy_texture(pfd.hw_rt);
			pfd.gpu_index_rt = NULL_GPU_INDEX;
		}
	}

	void editor_renderer::draw_vekt(u8 frame_index, const vekt::draw_buffer& buffer)
	{
		ZoneScoped;

		void*				  user_data		   = buffer.user_data;
		const vekt::id		  font			   = buffer.font_id;
		const vekt::id		  atlas			   = buffer.atlas_id;
		const vekt::font_type font_type		   = buffer.font_type;
		const vekt::vertex*	  buffer_vtx_start = buffer.vertex_start;
		const vekt::index*	  buffer_idx_start = buffer.index_start;
		const vector4		  clip			   = buffer.clip;
		const u32			  buffer_idx_count = buffer.index_count;
		const u32			  buffer_vtx_count = buffer.vertex_count;

		const gfx_id sdf_shader		= _shaders.gui_sdf;
		const gfx_id text_shader	= _shaders.gui_text;
		const gfx_id default_shader = _shaders.gui_default;
		const gfx_id texture_shader = _shaders.gui_texture;

		per_frame_data& pfd			= _pfd[frame_index];
		const u32		vtx_counter = pfd.counter_vtx;
		const u32		idx_counter = pfd.counter_idx;
		const u16		dc_index	= pfd.draw_call_count++;
		pfd.counter_vtx += buffer_vtx_count;
		pfd.counter_idx += buffer_idx_count;
		pfd.buf_gui_vtx.buffer_data(sizeof(vekt::vertex) * static_cast<size_t>(vtx_counter), buffer_vtx_start, static_cast<size_t>(buffer_vtx_count) * sizeof(vekt::vertex));
		pfd.buf_gui_idx.buffer_data(sizeof(vekt::index) * static_cast<size_t>(idx_counter), buffer_idx_start, static_cast<size_t>(buffer_idx_count) * sizeof(vekt::index));

		gui_draw_call& dc = _gui_draw_calls[dc_index];
		dc				  = {};
		dc.start_idx	  = static_cast<u16>(idx_counter);
		dc.start_vtx	  = static_cast<u16>(vtx_counter);
		dc.index_count	  = static_cast<u16>(buffer_idx_count);

		// Scissor
		if (clip.y < 0.0f)
		{
			dc.scissors.y = 0;
			dc.scissors.w = static_cast<u16>(math::max(0.0f, clip.w + clip.y));
		}
		else
		{
			dc.scissors.y = static_cast<u16>(clip.y);
			dc.scissors.w = static_cast<u16>(clip.w);
		}
		if (clip.x < 0.0f)
		{
			dc.scissors.x = 0;
			dc.scissors.z = math::min((u16)0, static_cast<u16>(clip.x + clip.z));
		}
		else
		{
			dc.scissors.x = static_cast<u16>(clip.x);
			dc.scissors.z = static_cast<u16>(clip.z);
		}

		if (font != NULL_WIDGET_ID)
		{
			dc.shader = font_type == vekt::font_type::sdf ? sdf_shader : text_shader;

			bool found = false;
			for (atlas_ref& ref : _gfx_data.atlases)
			{
				if (ref.atlas->get_id() == atlas)
				{
					dc.atlas_gpu_index = ref.texture_gpu_index;
					found			   = true;
					break;
				}
			}

			SFG_ASSERT(found);
		}
		else if (user_data != nullptr)
		{
			editor_gui_user_data* ud = reinterpret_cast<editor_gui_user_data*>(user_data);
			if (ud->type >= editor_gui_user_data_type::world_rt && ud->type <= editor_gui_user_data_type::bloom_rt)
			{
				dc.shader  = texture_shader;
				dc.ud_type = ud->type;
			}
			else
			{
				int a = 5;
			}
		}
		else
		{
			dc.shader = default_shader;
		}
	}

	void editor_renderer::on_atlas_created(vekt::atlas* atlas, void* user_data)
	{
		editor_renderer* r		 = static_cast<editor_renderer*>(user_data);
		gfx_backend*	 backend = gfx_backend::get();

		atlas_ref ref		  = {};
		ref.atlas			  = atlas;
		ref.texture			  = backend->create_texture({.texture_format = atlas->get_is_lcd() ? format::r8g8b8a8_srgb : format::r8_unorm,
														 .size			 = vector2ui16(static_cast<u16>(atlas->get_width()), static_cast<u16>(atlas->get_height())),
														 .flags			 = texture_flags::tf_sampled | texture_flags::tf_is_2d,
														 .views			 = {{.type = view_type::sampled}},
														 .debug_name	 = "editor_gui_atlas"});
		ref.texture_gpu_index = backend->get_texture_gpu_index(ref.texture, 0);

		const u32 txt_size		= backend->get_texture_size(atlas->get_width(), atlas->get_height(), atlas->get_is_lcd() ? 3 : 1);
		const u32 adjusted_size = backend->align_texture_size(txt_size);
		ref.intermediate_buffer = backend->create_resource({
			.size		= adjusted_size,
			.flags		= resource_flags::rf_cpu_visible,
			.debug_name = "editor_gui_inter_buffer",
		});

		r->_gfx_data.atlases.push_back(ref);
	}

	void editor_renderer::on_atlas_updated(vekt::atlas* atlas, void* user_data)
	{
		editor_renderer* r		 = static_cast<editor_renderer*>(user_data);
		gfx_backend*	 backend = gfx_backend::get();

		for (atlas_ref& ref : r->_gfx_data.atlases)
		{
			if (ref.atlas == atlas)
			{
				const unsigned char* data		  = atlas->get_data();
				const unsigned int	 size		  = atlas->get_data_size();
				const bool			 is_lcd		  = atlas->get_is_lcd();
				const u32			 atlas_width  = atlas->get_width();
				const u32			 atlas_height = atlas->get_height();
				const u8			 bpp		  = atlas->get_is_lcd() ? 3 : 1;

				u32 adjusted_size = 0;
				ref.buffer.pixels = reinterpret_cast<u8*>(backend->adjust_buffer_pitch((void*)data, atlas_width, atlas_height, bpp, adjusted_size));
				ref.buffer.size	  = vector2ui16(static_cast<u16>(atlas_width), static_cast<u16>(atlas_height));
				ref.buffer.bpp	  = bpp;

				static_vector<texture_buffer, MAX_TEXTURE_MIPS> buffers;
				buffers.push_back(ref.buffer);
				r->_gfx_data.texture_queue->add_request(buffers, ref.texture, ref.intermediate_buffer, 0, resource_state::resource_state_ps_resource, false);
				return;
			}
		}
	}

	void editor_renderer::on_atlas_destroyed(vekt::atlas* atlas, void* user_data)
	{
		editor_renderer* r		 = static_cast<editor_renderer*>(user_data);
		gfx_backend*	 backend = gfx_backend::get();

		for (atlas_ref& ref : r->_gfx_data.atlases)
		{
			if (ref.atlas == atlas)
			{
				backend->destroy_texture(ref.texture);
				backend->destroy_resource(ref.intermediate_buffer);
				r->_gfx_data.atlases.remove(ref);
				return;
			}
		}

		SFG_ASSERT(false);
	}
}
