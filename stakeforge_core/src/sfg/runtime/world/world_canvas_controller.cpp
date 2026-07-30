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

#include "world_canvas_controller.hpp"
#include "ecs.hpp"
#include "ecs_helpers.hpp"
#include "engine_components.hpp"
#include "system_components.hpp"
#include "world.hpp"

#include <sfg/data/fixed_vector.hpp>
#include <sfg/data/frame_vector.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/math/math.hpp>
#include <sfg/runtime/render/world_canvas_render_snapshot.hpp>
#include <sfg/runtime/resources/shader_types.hpp>
#include <sfg/runtime/scripting/script_runtime.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
#define CANVAS_DEFAULT_SHADER		 TO_SIDC("engine/resource_pack/shaders/canvas_ui_default.hlsl")
#define CANVAS_TEXTURE_SHADER		 TO_SIDC("engine/resource_pack/shaders/canvas_ui_texture.hlsl")
#define CANVAS_TEXT_GRAYSCALE_SHADER TO_SIDC("engine/resource_pack/shaders/canvas_ui_text_grayscale.hlsl")
#define CANVAS_TEXT_SDF_SHADER		 TO_SIDC("engine/resource_pack/shaders/canvas_ui_sdf.hlsl")

	struct world_canvas_controller_t::impl_t
	{
		enum class widget_kind_e : u8
		{
			none,
			frame,
			text,
			image,
			button,
		};

		struct widget_state_t
		{
			ui::widget_id_t label	   = NULL_WIDGET;
			u16				generation = 1;
			widget_kind_e	kind	   = widget_kind_e::none;
		};

		struct runtime_t
		{
			unique_t<ui::ui_context>	   context = {};
			fixed_vector_t<widget_state_t> widgets = {};
			fixed_vector_t<canvas_event_t> events  = {};
			component_canvas_t			   config  = {};
			entity_id_t					   entity  = NULL_ENTITY_ID;

			bool is_config_equal(const component_canvas_t& other) const
			{
				return config.default_font == other.default_font && config.text_pool_budget_bytes == other.text_pool_budget_bytes && config.vertex_pool_budget_bytes == other.vertex_pool_budget_bytes &&
					   config.index_pool_budget_bytes == other.index_pool_budget_bytes && config.sort_order == other.sort_order && config.ui_scale == other.ui_scale && config.max_widgets == other.max_widgets &&
					   config.draw_buffer_count == other.draw_buffer_count && config.render_stage == other.render_stage && config.input_enabled == other.input_enabled;
			}

			canvas_widget_handle_t make_handle(ui::widget_id_t id) const
			{
				return static_cast<canvas_widget_handle_t>(id) | (static_cast<canvas_widget_handle_t>(widgets[id].generation) << 16);
			}

			ui::widget_id_t resolve_handle(canvas_widget_handle_t handle) const
			{
				if (handle == NULL_CANVAS_WIDGET_HANDLE)
					return NULL_WIDGET;

				const ui::widget_id_t id		 = static_cast<ui::widget_id_t>(handle & 0xFFFFu);
				const u16			  generation = static_cast<u16>(handle >> 16);

				if (id >= widgets.size() || generation == 0 || widgets[id].generation != generation || !context->get_tree().is_alive(id))
					return NULL_WIDGET;

				return id;
			}

			void invalidate_recursive(ui::widget_id_t id)
			{
				ui::widget_id_t child = context->get_tree().node(id).first_child;

				while (child != NULL_WIDGET)
				{
					const ui::widget_id_t next = context->get_tree().node(child).next_sibling;
					invalidate_recursive(child);
					child = next;
				}

				widget_state_t& state = widgets[id];
				state.generation++;

				if (state.generation == 0)
					state.generation = 1;

				state.label = NULL_WIDGET;
				state.kind	= widget_kind_e::none;
			}

			void queue_event(
				ui::widget_id_t id, canvas_event_type_e type, const vec2f_t& position = vec2f_t::zero, const vec2f_t& delta = vec2f_t::zero, u8 button = UINT8_MAX, u16 key = 0, u16 scan_code = 0, u8 action = 0, f32 wheel_delta = 0.0f, bool from_nav = false)
			{
				events.push_back({
					.position	 = position,
					.delta		 = delta,
					.widget		 = make_handle(id),
					.canvas		 = entity,
					.wheel_delta = wheel_delta,
					.key		 = key,
					.scan_code	 = scan_code,
					.type		 = type,
					.button		 = button,
					.action		 = action,
					.from_nav	 = from_nav ? 1u : 0u,
				});
			}
		};

		static void on_press(ui::input_router_t&, ui::widget_id_t id, const vec2f_t& position, ui::mouse_button_e button, void* user_data)
		{
			static_cast<runtime_t*>(user_data)->queue_event(id, canvas_event_type_e::press, position, vec2f_t::zero, static_cast<u8>(button));
		}

		static void on_release(ui::input_router_t&, ui::widget_id_t id, const vec2f_t& position, ui::mouse_button_e button, void* user_data)
		{
			static_cast<runtime_t*>(user_data)->queue_event(id, canvas_event_type_e::release, position, vec2f_t::zero, static_cast<u8>(button));
		}

		static void on_click(ui::input_router_t&, ui::widget_id_t id, const vec2f_t& position, ui::mouse_button_e button, void* user_data)
		{
			static_cast<runtime_t*>(user_data)->queue_event(id, canvas_event_type_e::click, position, vec2f_t::zero, static_cast<u8>(button));
		}

		static void on_double_click(ui::input_router_t&, ui::widget_id_t id, const vec2f_t& position, ui::mouse_button_e button, void* user_data)
		{
			static_cast<runtime_t*>(user_data)->queue_event(id, canvas_event_type_e::double_click, position, vec2f_t::zero, static_cast<u8>(button));
		}

		static void on_hover_enter(ui::input_router_t&, ui::widget_id_t id, const vec2f_t& position, const vec2f_t& delta, void* user_data)
		{
			static_cast<runtime_t*>(user_data)->queue_event(id, canvas_event_type_e::hover_enter, position, delta);
		}

		static void on_hover_exit(ui::input_router_t&, ui::widget_id_t id, const vec2f_t& position, const vec2f_t& delta, void* user_data)
		{
			static_cast<runtime_t*>(user_data)->queue_event(id, canvas_event_type_e::hover_exit, position, delta);
		}

		static void on_hover_move(ui::input_router_t&, ui::widget_id_t id, const vec2f_t& position, const vec2f_t& delta, void* user_data)
		{
			static_cast<runtime_t*>(user_data)->queue_event(id, canvas_event_type_e::hover_move, position, delta);
		}

		static void on_drag_begin(ui::input_router_t&, ui::widget_id_t id, const vec2f_t& position, const vec2f_t& delta, void* user_data)
		{
			static_cast<runtime_t*>(user_data)->queue_event(id, canvas_event_type_e::drag_begin, position, delta);
		}

		static void on_drag(ui::input_router_t&, ui::widget_id_t id, const vec2f_t& position, const vec2f_t& delta, void* user_data)
		{
			static_cast<runtime_t*>(user_data)->queue_event(id, canvas_event_type_e::drag, position, delta);
		}

		static void on_drag_end(ui::input_router_t&, ui::widget_id_t id, const vec2f_t& position, const vec2f_t& delta, void* user_data)
		{
			static_cast<runtime_t*>(user_data)->queue_event(id, canvas_event_type_e::drag_end, position, delta);
		}

		static void on_focus_gain(ui::input_router_t&, ui::widget_id_t id, bool from_nav, void* user_data)
		{
			static_cast<runtime_t*>(user_data)->queue_event(id, canvas_event_type_e::focus_gain, vec2f_t::zero, vec2f_t::zero, UINT8_MAX, 0, 0, 0, 0.0f, from_nav);
		}

		static void on_focus_lose(ui::input_router_t&, ui::widget_id_t id, bool from_nav, void* user_data)
		{
			static_cast<runtime_t*>(user_data)->queue_event(id, canvas_event_type_e::focus_lose, vec2f_t::zero, vec2f_t::zero, UINT8_MAX, 0, 0, 0, 0.0f, from_nav);
		}

		static void on_key(ui::input_router_t&, ui::widget_id_t id, const ui::key_event_t& event, void* user_data)
		{
			static_cast<runtime_t*>(user_data)->queue_event(id, canvas_event_type_e::key, vec2f_t::zero, vec2f_t::zero, UINT8_MAX, event.key, event.scan_code, static_cast<u8>(event.action));
		}

		static void on_wheel(ui::input_router_t& router, ui::widget_id_t id, f32 delta, void* user_data)
		{
			static_cast<runtime_t*>(user_data)->queue_event(id, canvas_event_type_e::wheel, router.get_mouse_position(), vec2f_t::zero, UINT8_MAX, 0, 0, 0, delta);
		}

		runtime_t* find_runtime(entity_id_t entity)
		{
			const ecs_component_table_t&	 system_table = world->get_component_table(type_id_t<component_system_canvas_t>::value);
			const component_system_canvas_t* system		  = ecs_helpers_t::table_find_as_const<component_system_canvas_t>(system_table, entity);

			if (system == nullptr)
				return nullptr;

			SFG_ASSERT(system->runtime_index < runtimes.size());
			SFG_ASSERT(runtimes[system->runtime_index]->entity == entity);
			return runtimes[system->runtime_index].get();
		}

		const runtime_t* find_runtime(entity_id_t entity) const
		{
			return const_cast<impl_t*>(this)->find_runtime(entity);
		}

		void create_canvas(entity_id_t entity, const component_canvas_t& component)
		{
			runtimes.push_back(make_unique<runtime_t>());
			runtime_t& runtime = *runtimes.back();
			runtime.context	   = make_unique<ui::ui_context>();
			runtime.config	   = component;
			runtime.entity	   = entity;
			runtime.widgets.init(component.max_widgets);
			runtime.widgets.resize(component.max_widgets);
			runtime.events.init(static_cast<u32>(component.max_widgets) * 2u);

			const u32 pipeline_flags = component.render_stage == canvas_render_stage_e::before_post_process ? shader_variant_flags_post_process_hdr : shader_variant_flags_post_process_ldr;
			const f32 dpi_scale		 = world->get_screen().get_dpi_scale();

			runtime.context->init({
				.canvas =
					{
						.vertex_pool_budget_bytes		= component.vertex_pool_budget_bytes,
						.index_pool_budget_bytes		= component.index_pool_budget_bytes,
						.buffer_count					= component.draw_buffer_count,
						.geometry_span_count			= static_cast<u32>(component.max_widgets) * 8u,
						.text_cache_vertex_budget_bytes = component.vertex_pool_budget_bytes,
						.text_cache_index_budget_bytes	= component.index_pool_budget_bytes,
						.clip_stack_initial_capacity	= math::min<u32>(component.max_widgets, 64u),
						.text_cache_initial_capacity	= component.max_widgets,
						.path_initial_capacity			= 256,
					},
				.user_ui_scale			   = component.ui_scale,
				.dpi_scale				   = dpi_scale,
				.max_widgets			   = component.max_widgets,
				.text_pool_budget_bytes	   = component.text_pool_budget_bytes,
				.snapshot_vertex_max_bytes = component.vertex_pool_budget_bytes,
				.snapshot_index_max_bytes  = component.index_pool_budget_bytes,
				.pipeline_variant_flags	   = pipeline_flags,
				.render_snapshots_enabled  = false,
			});
			runtime.context->get_paint().set_pipelines({
				.default_pipeline		 = CANVAS_DEFAULT_SHADER,
				.text_pipeline			 = CANVAS_TEXT_GRAYSCALE_SHADER,
				.grayscale_text_pipeline = CANVAS_TEXT_GRAYSCALE_SHADER,
				.sdf_pipeline			 = CANVAS_TEXT_SDF_SHADER,
			});

			ecs_component_table_t&	   system_table = world->get_component_table(type_id_t<component_system_canvas_t>::value);
			component_system_canvas_t& system		= ecs_helpers_t::table_add_or_get_as<component_system_canvas_t>(system_table, entity);
			system.runtime_index					= static_cast<u32>(runtimes.size() - 1);
		}

		void destroy_canvas(entity_id_t entity)
		{
			ecs_component_table_t&	   system_table	 = world->get_component_table(type_id_t<component_system_canvas_t>::value);
			component_system_canvas_t& system		 = ecs_helpers_t::table_get_as<component_system_canvas_t>(system_table, entity);
			const u32				   runtime_index = system.runtime_index;
			runtime_t&				   runtime		 = *runtimes[runtime_index];

			runtime.context->uninit();
			runtime.context.reset();
			runtime.events.uninit();
			runtime.widgets.uninit();

			const u32 last_index = static_cast<u32>(runtimes.size() - 1);

			if (runtime_index != last_index)
			{
				runtimes[runtime_index] = std::move(runtimes[last_index]);

				component_system_canvas_t& moved_system = ecs_helpers_t::table_get_as<component_system_canvas_t>(system_table, runtimes[runtime_index]->entity);
				moved_system.runtime_index				= runtime_index;
			}

			runtimes.pop_back();
			ecs_t::table_remove(system_table, entity);
		}

		vec2f_t map_position(const vec2f_t& position) const
		{
			return world->get_screen().screen_to_render_position(position);
		}

		void append_runtime(const runtime_t& runtime, world_canvas_draw_snapshot_t& destination) const
		{
			const ui::vg_canvas_t& canvas = runtime.context->get_canvas();

			for (const ui::vg_draw_buffer_t& source : canvas.get_draw_buffers())
			{
				if (source.vertex_count == 0 || source.index_count == 0)
					continue;

				const u32 vertex_offset = static_cast<u32>(destination.vertices.size());
				const u32 index_offset	= static_cast<u32>(destination.indices.size());

				destination.vertices.resize(destination.vertices.size() + source.vertex_count);
				destination.indices.resize(destination.indices.size() + source.index_count);

				canvas.copy_draw_buffer_vertices(source, destination.vertices.data() + vertex_offset);
				canvas.copy_draw_buffer_indices(source, destination.indices.data() + index_offset);

				destination.draw_buffers.push_back({
					.resolved	   = source.resolved,
					.clip		   = source.clip,
					.draw_order	   = source.draw_order,
					.vertex_count  = source.vertex_count,
					.index_count   = source.index_count,
					.vertex_offset = vertex_offset,
					.index_offset  = index_offset,
				});
			}
		}

		vector_t<unique_t<runtime_t>> runtimes = {};
		world_t*					  world	   = nullptr;
	};

	world_canvas_controller_t::world_canvas_controller_t() : _impl(make_unique<impl_t>())
	{
	}

	world_canvas_controller_t::~world_canvas_controller_t() = default;

	void world_canvas_controller_t::init(world_t& world)
	{
		_impl->world = &world;
		_impl->runtimes.reserve(4);
	}

	void world_canvas_controller_t::uninit()
	{
		clear();
		_impl->runtimes.resize(0);
		_impl->world = nullptr;
	}

	void world_canvas_controller_t::clear()
	{
		ecs_component_table_t& system_table = _impl->world->get_component_table(type_id_t<component_system_canvas_t>::value);

		while (!_impl->runtimes.empty())
			_impl->destroy_canvas(_impl->runtimes.back()->entity);

		SFG_ASSERT(ecs_t::is_table_empty(system_table));
	}

	void world_canvas_controller_t::clear_widgets()
	{
		for (const unique_t<impl_t::runtime_t>& runtime : _impl->runtimes)
			clear_widgets(runtime->entity);
	}

	void world_canvas_controller_t::destroy_entity(entity_id_t entity)
	{
		const ecs_component_table_t& system_table = _impl->world->get_component_table(type_id_t<component_system_canvas_t>::value);

		if (!ecs_t::table_has(system_table, entity))
			return;

		_impl->destroy_canvas(entity);
	}

	void world_canvas_controller_t::sync_create_destroy_canvases()
	{
		ecs_component_table_t&		 system_table	= _impl->world->get_component_table(type_id_t<component_system_canvas_t>::value);
		const ecs_component_table_t& canvas_table	= _impl->world->get_component_table(type_id_t<component_canvas_t>::value);
		const ecs_component_table_t& disabled_table = _impl->world->get_component_table(type_id_t<component_disabled_t>::value);

		frame_vector_t<entity_id_t> destroy_entities = {};

		{
			const ecs_component_table_ref_t table_refs[] = {
				system_table.ref(),
				!canvas_table.ref(),
			};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
				destroy_entities.push_back(row.id);
		}

		{
			const ecs_component_table_ref_t table_refs[] = {
				system_table.ref(),
				disabled_table.ref(),
			};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
				destroy_entities.push_back(row.id);
		}

		{
			const ecs_component_table_ref_t table_refs[] = {
				system_table.ref(),
				canvas_table.ref(),
			};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
			{
				const component_system_canvas_t& system	   = ecs_helpers_t::row_get<component_system_canvas_t>(row, 0);
				const component_canvas_t&		 component = ecs_helpers_t::row_get<component_canvas_t>(row, 1);

				if (!_impl->runtimes[system.runtime_index]->is_config_equal(component))
					destroy_entities.push_back(row.id);
			}
		}

		std::sort(destroy_entities.begin(), destroy_entities.end());
		destroy_entities.erase(std::unique(destroy_entities.begin(), destroy_entities.end()), destroy_entities.end());

		for (const entity_id_t entity : destroy_entities)
			_impl->destroy_canvas(entity);

		const ecs_component_table_ref_t create_refs[] = {
			canvas_table.ref(),
			!disabled_table.ref(),
			!system_table.ref(),
		};
		frame_vector_t<entity_id_t> create_entities = {};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = create_refs, .size = std::size(create_refs)}))
			create_entities.push_back(row.id);

		for (const entity_id_t entity : create_entities)
		{
			const component_canvas_t& component = ecs_helpers_t::table_get_as_const<component_canvas_t>(canvas_table, entity);
			_impl->create_canvas(entity, component);
		}
	}

	void world_canvas_controller_t::tick(f32 dt_seconds)
	{
		sync_create_destroy_canvases();

		const world_screen_t& screen	  = _impl->world->get_screen();
		const vec2u16_t		  render_size = screen.get_render_size();
		const vec4f_t		  screen_rect = {0.0f, 0.0f, static_cast<f32>(render_size.x), static_cast<f32>(render_size.y)};

		for (const unique_t<impl_t::runtime_t>& runtime : _impl->runtimes)
			runtime->context->tick(screen_rect, screen.get_dpi_scale(), dt_seconds);
	}

	void world_canvas_controller_t::write_render_snapshot(world_canvas_render_snapshot_t& snapshot) const
	{
		snapshot.clear();

		frame_vector_t<const impl_t::runtime_t*> sorted = {};
		sorted.reserve(_impl->runtimes.size());

		for (const unique_t<impl_t::runtime_t>& runtime : _impl->runtimes)
			sorted.push_back(runtime.get());

		std::stable_sort(sorted.begin(), sorted.end(), [](const impl_t::runtime_t* left, const impl_t::runtime_t* right) { return left->config.sort_order < right->config.sort_order; });

		for (const impl_t::runtime_t* runtime : sorted)
		{
			world_canvas_draw_snapshot_t& destination = runtime->config.render_stage == canvas_render_stage_e::before_post_process ? snapshot.before_post_process : snapshot.after_post_process;
			_impl->append_runtime(*runtime, destination);
		}
	}

	bool world_canvas_controller_t::key_event(u16 key, u16 scan_code, u8 action)
	{
		for (const unique_t<impl_t::runtime_t>& runtime_ptr : _impl->runtimes)
		{
			impl_t::runtime_t& runtime = *runtime_ptr;

			if (!runtime.config.input_enabled || runtime.context->get_input().get_focused() == NULL_WIDGET)
				continue;

			runtime.context->on_key({
				.key	   = key,
				.scan_code = scan_code,
				.action	   = static_cast<ui::key_action_e>(action),
			});
			return true;
		}

		return false;
	}

	bool world_canvas_controller_t::mouse_button_event(u8 button, u8 action, const vec2f_t& position)
	{
		if (button >= static_cast<u8>(ui::mouse_button_e::count))
			return false;

		const vec2f_t			 mapped_position = _impl->map_position(position);
		const ui::mouse_button_e mapped_button	 = static_cast<ui::mouse_button_e>(button);
		impl_t::runtime_t*		 target			 = nullptr;

		for (const unique_t<impl_t::runtime_t>& runtime_ptr : _impl->runtimes)
		{
			impl_t::runtime_t& runtime = *runtime_ptr;

			if (!runtime.config.input_enabled)
				continue;

			runtime.context->on_mouse_move(mapped_position);

			const bool is_target = action == 0 ? runtime.context->get_input().get_hovered() != NULL_WIDGET : runtime.context->get_input().is_pressed(mapped_button) != NULL_WIDGET;

			if (!is_target)
				continue;

			if (target == nullptr || runtime.config.render_stage > target->config.render_stage || (runtime.config.render_stage == target->config.render_stage && runtime.config.sort_order >= target->config.sort_order))
				target = &runtime;
		}

		if (action == 0)
		{
			for (const unique_t<impl_t::runtime_t>& runtime : _impl->runtimes)
			{
				if (runtime.get() != target)
					runtime->context->get_input().set_focus(NULL_WIDGET, false);
			}
		}

		if (target == nullptr)
			return false;

		target->context->on_mouse_button(mapped_button, action == 0);
		return true;
	}

	bool world_canvas_controller_t::mouse_move_event(const vec2f_t& position)
	{
		const vec2f_t mapped_position = _impl->map_position(position);
		bool		  consumed		  = false;

		for (const unique_t<impl_t::runtime_t>& runtime_ptr : _impl->runtimes)
		{
			impl_t::runtime_t& runtime = *runtime_ptr;

			if (!runtime.config.input_enabled)
				continue;

			runtime.context->on_mouse_move(mapped_position);
			consumed |= runtime.context->get_input().get_hovered() != NULL_WIDGET;
		}

		return consumed;
	}

	bool world_canvas_controller_t::mouse_wheel_event(const vec2f_t& position, f32 delta)
	{
		const vec2f_t	   mapped_position = _impl->map_position(position);
		impl_t::runtime_t* target		   = nullptr;

		for (const unique_t<impl_t::runtime_t>& runtime_ptr : _impl->runtimes)
		{
			impl_t::runtime_t& runtime = *runtime_ptr;

			if (!runtime.config.input_enabled)
				continue;

			runtime.context->on_mouse_move(mapped_position);

			if (runtime.context->get_input().get_hovered() == NULL_WIDGET)
				continue;

			if (target == nullptr || runtime.config.render_stage > target->config.render_stage || (runtime.config.render_stage == target->config.render_stage && runtime.config.sort_order >= target->config.sort_order))
				target = &runtime;
		}

		if (target == nullptr)
			return false;

		target->context->on_wheel(delta);
		return true;
	}

	bool world_canvas_controller_t::is_keyboard_focus_active() const
	{
		for (const unique_t<impl_t::runtime_t>& runtime : _impl->runtimes)
		{
			if (runtime->config.input_enabled && runtime->context->get_input().get_focused() != NULL_WIDGET)
				return true;
		}

		return false;
	}

	void world_canvas_controller_t::dispatch_events(void* world_script_instance)
	{
		for (const unique_t<impl_t::runtime_t>& runtime_ptr : _impl->runtimes)
		{
			impl_t::runtime_t& runtime = *runtime_ptr;

			if (world_script_instance != nullptr)
			{
				for (const canvas_event_t& event : runtime.events)
					script_runtime_t::get().canvas_event_world_script(world_script_instance, event);
			}

			runtime.events.resize(0);
		}
	}

	canvas_widget_handle_t world_canvas_controller_t::create_frame(entity_id_t canvas, canvas_widget_handle_t parent, const ui::layout_in_t& layout, const ui::vg_rect_paint_t& style)
	{
		impl_t::runtime_t* runtime = _impl->find_runtime(canvas);

		if (runtime == nullptr)
			return NULL_CANVAS_WIDGET_HANDLE;

		const ui::widget_id_t parent_id = parent == NULL_CANVAS_WIDGET_HANDLE ? runtime->context->get_root() : runtime->resolve_handle(parent);

		if (parent_id == NULL_WIDGET)
			return NULL_CANVAS_WIDGET_HANDLE;

		const ui::widget_id_t id  = runtime->context->allocate_widget();
		runtime->widgets[id].kind = impl_t::widget_kind_e::frame;
		runtime->context->get_tree().attach(parent_id, id);
		runtime->context->get_tree().in(id) = layout;
		runtime->context->get_tree().mark_layout_dirty();
		runtime->context->get_paint().set_rect(id, style);
		return runtime->make_handle(id);
	}

	canvas_widget_handle_t world_canvas_controller_t::create_text(entity_id_t canvas, canvas_widget_handle_t parent, const ui::layout_in_t& layout, const char* text, const ui::vg_text_style_t& style)
	{
		impl_t::runtime_t* runtime = _impl->find_runtime(canvas);

		if (runtime == nullptr)
			return NULL_CANVAS_WIDGET_HANDLE;

		const ui::widget_id_t parent_id = parent == NULL_CANVAS_WIDGET_HANDLE ? runtime->context->get_root() : runtime->resolve_handle(parent);

		if (parent_id == NULL_WIDGET)
			return NULL_CANVAS_WIDGET_HANDLE;

		const ui::widget_id_t id  = runtime->context->allocate_widget();
		runtime->widgets[id].kind = impl_t::widget_kind_e::text;
		runtime->context->get_tree().attach(parent_id, id);
		runtime->context->get_tree().in(id) = layout;
		runtime->context->get_tree().mark_layout_dirty();
		runtime->context->set_widget_text(id, text);

		ui::vg_text_style_t resolved_style = style;

		if (resolved_style.font == NULL_RESOURCE_HANDLE)
			resolved_style.font = runtime->config.default_font;

		runtime->context->get_paint().set_text(id, runtime->context->widget_text(id), runtime->context->widget_text_len(id), resolved_style);
		return runtime->make_handle(id);
	}

	canvas_widget_handle_t world_canvas_controller_t::create_image(entity_id_t canvas, canvas_widget_handle_t parent, const ui::layout_in_t& layout, resource_handle_t texture, const vec4f_t& tint)
	{
		impl_t::runtime_t* runtime = _impl->find_runtime(canvas);

		if (runtime == nullptr)
			return NULL_CANVAS_WIDGET_HANDLE;

		const ui::widget_id_t parent_id = parent == NULL_CANVAS_WIDGET_HANDLE ? runtime->context->get_root() : runtime->resolve_handle(parent);

		if (parent_id == NULL_WIDGET)
			return NULL_CANVAS_WIDGET_HANDLE;

		const ui::widget_id_t id  = runtime->context->allocate_widget();
		runtime->widgets[id].kind = impl_t::widget_kind_e::image;
		runtime->context->get_tree().attach(parent_id, id);
		runtime->context->get_tree().in(id) = layout;
		runtime->context->get_tree().mark_layout_dirty();
		set_image(canvas, runtime->make_handle(id), texture, tint);
		return runtime->make_handle(id);
	}

	canvas_widget_handle_t world_canvas_controller_t::create_button(
		entity_id_t canvas, canvas_widget_handle_t parent, const ui::layout_in_t& layout, const char* text, const ui::vg_rect_paint_t& frame_style, const ui::vg_text_style_t& text_style, const vec4f_t& hover_color, const vec4f_t& press_color)
	{
		impl_t::runtime_t* runtime = _impl->find_runtime(canvas);

		if (runtime == nullptr)
			return NULL_CANVAS_WIDGET_HANDLE;

		const ui::widget_id_t parent_id = parent == NULL_CANVAS_WIDGET_HANDLE ? runtime->context->get_root() : runtime->resolve_handle(parent);

		if (parent_id == NULL_WIDGET)
			return NULL_CANVAS_WIDGET_HANDLE;

		const ui::widget_id_t id  = runtime->context->allocate_widget();
		runtime->widgets[id].kind = impl_t::widget_kind_e::button;
		runtime->context->get_tree().attach(parent_id, id);
		runtime->context->get_tree().in(id) = layout;
		runtime->context->get_tree().in(id).flags |= ui::wf_input | ui::wf_focusable;
		runtime->context->get_tree().mark_layout_dirty();
		runtime->context->get_paint().set_rect(id, frame_style);
		runtime->context->get_paint().set_hover_color(id, hover_color);
		runtime->context->get_paint().set_press_color(id, press_color);

		const ui::listener_bundle_t listener = {
			.on_press		 = impl_t::on_press,
			.on_release		 = impl_t::on_release,
			.on_click		 = impl_t::on_click,
			.on_double_click = impl_t::on_double_click,
			.on_hover_enter	 = impl_t::on_hover_enter,
			.on_hover_exit	 = impl_t::on_hover_exit,
			.on_hover_move	 = impl_t::on_hover_move,
			.on_drag_begin	 = impl_t::on_drag_begin,
			.on_drag		 = impl_t::on_drag,
			.on_drag_end	 = impl_t::on_drag_end,
			.on_focus_gain	 = impl_t::on_focus_gain,
			.on_focus_lose	 = impl_t::on_focus_lose,
			.on_key			 = impl_t::on_key,
			.on_wheel		 = impl_t::on_wheel,
			.user_data		 = runtime,
		};
		runtime->context->get_input().set_listener(id, listener);

		const ui::widget_id_t label	 = runtime->context->allocate_widget();
		runtime->widgets[label].kind = impl_t::widget_kind_e::text;
		runtime->widgets[id].label	 = label;
		runtime->context->get_tree().attach(id, label);
		runtime->context->get_tree().draw_order(label) = runtime->context->get_tree().draw_order_const(id) + 1;

		ui::layout_in_t& label_layout = runtime->context->get_tree().in(label);
		label_layout.pos_mode_x		  = ui::pos_mode_e::relative_in_parent;
		label_layout.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		label_layout.pos_value		  = {0.5f, 0.5f};
		label_layout.anchor_x		  = ui::anchor_e::center;
		label_layout.anchor_y		  = ui::anchor_e::center;
		runtime->context->set_widget_text(label, text);

		ui::vg_text_style_t resolved_style = text_style;

		if (resolved_style.font == NULL_RESOURCE_HANDLE)
			resolved_style.font = runtime->config.default_font;

		runtime->context->get_paint().set_text(label, runtime->context->widget_text(label), runtime->context->widget_text_len(label), resolved_style);
		return runtime->make_handle(id);
	}

	bool world_canvas_controller_t::destroy_widget(entity_id_t canvas, canvas_widget_handle_t widget)
	{
		impl_t::runtime_t* runtime = _impl->find_runtime(canvas);

		if (runtime == nullptr)
			return false;

		const ui::widget_id_t id = runtime->resolve_handle(widget);

		if (id == NULL_WIDGET)
			return false;

		runtime->invalidate_recursive(id);
		runtime->context->deallocate_widget(id);
		return true;
	}

	bool world_canvas_controller_t::clear_widgets(entity_id_t canvas)
	{
		impl_t::runtime_t* runtime = _impl->find_runtime(canvas);

		if (runtime == nullptr)
			return false;

		const ui::widget_id_t root	= runtime->context->get_root();
		ui::widget_id_t		  child = runtime->context->get_tree().node(root).first_child;

		while (child != NULL_WIDGET)
		{
			const ui::widget_id_t next = runtime->context->get_tree().node(child).next_sibling;
			runtime->invalidate_recursive(child);
			runtime->context->deallocate_widget(child);
			child = next;
		}

		return true;
	}

	bool world_canvas_controller_t::set_layout(entity_id_t canvas, canvas_widget_handle_t widget, const ui::layout_in_t& layout)
	{
		impl_t::runtime_t* runtime = _impl->find_runtime(canvas);

		if (runtime == nullptr)
			return false;

		const ui::widget_id_t id = runtime->resolve_handle(widget);

		if (id == NULL_WIDGET)
			return false;

		runtime->context->get_tree().in(id) = layout;
		runtime->context->get_tree().mark_layout_dirty();
		return true;
	}

	bool world_canvas_controller_t::set_visible(entity_id_t canvas, canvas_widget_handle_t widget, bool visible)
	{
		impl_t::runtime_t* runtime = _impl->find_runtime(canvas);

		if (runtime == nullptr)
			return false;

		const ui::widget_id_t id = runtime->resolve_handle(widget);

		if (id == NULL_WIDGET)
			return false;

		runtime->context->get_tree().set_visible(id, visible);
		return true;
	}

	bool world_canvas_controller_t::set_enabled(entity_id_t canvas, canvas_widget_handle_t widget, bool enabled)
	{
		impl_t::runtime_t* runtime = _impl->find_runtime(canvas);

		if (runtime == nullptr)
			return false;

		const ui::widget_id_t id = runtime->resolve_handle(widget);

		if (id == NULL_WIDGET)
			return false;

		ui::layout_in_t& layout = runtime->context->get_tree().in(id);

		if (enabled)
			layout.flags &= ~ui::wf_disabled;
		else
			layout.flags |= ui::wf_disabled;

		return true;
	}

	bool world_canvas_controller_t::set_text(entity_id_t canvas, canvas_widget_handle_t widget, const char* text)
	{
		impl_t::runtime_t* runtime = _impl->find_runtime(canvas);

		if (runtime == nullptr)
			return false;

		ui::widget_id_t id = runtime->resolve_handle(widget);

		if (id == NULL_WIDGET)
			return false;

		if (runtime->widgets[id].kind == impl_t::widget_kind_e::button)
			id = runtime->widgets[id].label;

		if (runtime->widgets[id].kind != impl_t::widget_kind_e::text)
			return false;

		runtime->context->set_widget_text(id, text);
		return true;
	}

	bool world_canvas_controller_t::set_frame_style(entity_id_t canvas, canvas_widget_handle_t widget, const ui::vg_rect_paint_t& style)
	{
		impl_t::runtime_t* runtime = _impl->find_runtime(canvas);

		if (runtime == nullptr)
			return false;

		const ui::widget_id_t id = runtime->resolve_handle(widget);

		if (id == NULL_WIDGET || (runtime->widgets[id].kind != impl_t::widget_kind_e::frame && runtime->widgets[id].kind != impl_t::widget_kind_e::button))
			return false;

		runtime->context->get_paint().set_rect(id, style);
		return true;
	}

	bool world_canvas_controller_t::set_text_style(entity_id_t canvas, canvas_widget_handle_t widget, const ui::vg_text_style_t& style)
	{
		impl_t::runtime_t* runtime = _impl->find_runtime(canvas);

		if (runtime == nullptr)
			return false;

		ui::widget_id_t id = runtime->resolve_handle(widget);

		if (id == NULL_WIDGET)
			return false;

		if (runtime->widgets[id].kind == impl_t::widget_kind_e::button)
			id = runtime->widgets[id].label;

		if (runtime->widgets[id].kind != impl_t::widget_kind_e::text)
			return false;

		ui::vg_text_style_t resolved_style = style;

		if (resolved_style.font == NULL_RESOURCE_HANDLE)
			resolved_style.font = runtime->config.default_font;

		runtime->context->get_paint().set_text(id, runtime->context->widget_text(id), runtime->context->widget_text_len(id), resolved_style);
		return true;
	}

	bool world_canvas_controller_t::set_image(entity_id_t canvas, canvas_widget_handle_t widget, resource_handle_t texture, const vec4f_t& tint)
	{
		impl_t::runtime_t* runtime = _impl->find_runtime(canvas);

		if (runtime == nullptr)
			return false;

		const ui::widget_id_t id = runtime->resolve_handle(widget);

		if (id == NULL_WIDGET || runtime->widgets[id].kind != impl_t::widget_kind_e::image)
			return false;

		ui::vg_rect_paint_t rect = {};
		rect.fill_color_a		 = tint;
		rect.fill_color_b		 = tint;

		ui::ui_render_state_t state = {};
		state.pipeline				= CANVAS_TEXTURE_SHADER;
		state.constants[0].handle	= texture;
		state.constants[0].type		= ui::ui_resource_type_e::texture;
		runtime->context->get_paint().set_rect(id, rect, state);
		return true;
	}

#undef CANVAS_DEFAULT_SHADER
#undef CANVAS_TEXTURE_SHADER
#undef CANVAS_TEXT_GRAYSCALE_SHADER
#undef CANVAS_TEXT_SDF_SHADER
}
