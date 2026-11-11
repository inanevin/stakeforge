/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.
*/

#include "world_canvas_controller.hpp"
#include "world_canvas_render_snapshot.hpp"
#include "core/runtime/world/ecs.hpp"
#include "core/runtime/world/engine_components.hpp"
#include "core/runtime/world/system_components.hpp"
#include "core/runtime/world/world.hpp"

#include "core/data/fixed_vector.hpp"
#include "core/data/frame_vector.hpp"
#include "core/io/assert.hpp"
#include "core/math/math.hpp"
#include "core/runtime/resources/shader_types.hpp"
#include "core/runtime/plugin/game_plugin_runtime.hpp"
#include "core/runtime/ui/ui_context.hpp"

namespace sfg
{
	enum class canvas_widget_kind_e : u8
	{
		none,
		frame,
		text,
		image,
		button,
	};

	struct canvas_widget_state_t
	{
		ui::widget_id_t		 label		= NULL_WIDGET;
		u16					 generation = 1;
		canvas_widget_kind_e kind		= canvas_widget_kind_e::none;
	};

	struct world_canvas_events_t
	{
		fixed_vector_t<canvas_event_t> events  = {};
		canvas_widget_state_t*		   widgets = nullptr;
		entity_id_t					   entity  = NULL_ENTITY_ID;

		void queue_event(
			ui::widget_id_t id, canvas_event_type_e type, const vec2f_t& position = vec2f_t::zero, const vec2f_t& delta = vec2f_t::zero, u8 button = UINT8_MAX, u16 key = 0, u16 scan_code = 0, u8 action = 0, f32 wheel_delta = 0.0f, bool from_nav = false)
		{
			events.push_back({
				.position	 = position,
				.delta		 = delta,
				.widget		 = static_cast<canvas_widget_handle_t>(id) | (static_cast<canvas_widget_handle_t>(widgets[id].generation) << 16),
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

		static void on_press(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& position, ui::mouse_button_e button, void* user_data)
		{
			static_cast<world_canvas_events_t*>(user_data)->queue_event(id, canvas_event_type_e::press, position, vec2f_t::zero, static_cast<u8>(button));
		}

		static void on_release(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& position, ui::mouse_button_e button, void* user_data)
		{
			static_cast<world_canvas_events_t*>(user_data)->queue_event(id, canvas_event_type_e::release, position, vec2f_t::zero, static_cast<u8>(button));
		}

		static void on_click(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& position, ui::mouse_button_e button, void* user_data)
		{
			static_cast<world_canvas_events_t*>(user_data)->queue_event(id, canvas_event_type_e::click, position, vec2f_t::zero, static_cast<u8>(button));
		}

		static void on_double_click(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& position, ui::mouse_button_e button, void* user_data)
		{
			static_cast<world_canvas_events_t*>(user_data)->queue_event(id, canvas_event_type_e::double_click, position, vec2f_t::zero, static_cast<u8>(button));
		}

		static void on_hover_enter(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& position, const vec2f_t& delta, void* user_data)
		{
			static_cast<world_canvas_events_t*>(user_data)->queue_event(id, canvas_event_type_e::hover_enter, position, delta);
		}

		static void on_hover_exit(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& position, const vec2f_t& delta, void* user_data)
		{
			static_cast<world_canvas_events_t*>(user_data)->queue_event(id, canvas_event_type_e::hover_exit, position, delta);
		}

		static void on_hover_move(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& position, const vec2f_t& delta, void* user_data)
		{
			static_cast<world_canvas_events_t*>(user_data)->queue_event(id, canvas_event_type_e::hover_move, position, delta);
		}

		static void on_drag_begin(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& position, const vec2f_t& delta, void* user_data)
		{
			static_cast<world_canvas_events_t*>(user_data)->queue_event(id, canvas_event_type_e::drag_begin, position, delta);
		}

		static void on_drag(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& position, const vec2f_t& delta, void* user_data)
		{
			static_cast<world_canvas_events_t*>(user_data)->queue_event(id, canvas_event_type_e::drag, position, delta);
		}

		static void on_drag_end(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& position, const vec2f_t& delta, void* user_data)
		{
			static_cast<world_canvas_events_t*>(user_data)->queue_event(id, canvas_event_type_e::drag_end, position, delta);
		}

		static void on_focus_gain(ui::input_router_t& router, ui::widget_id_t id, bool from_nav, void* user_data)
		{
			static_cast<world_canvas_events_t*>(user_data)->queue_event(id, canvas_event_type_e::focus_gain, vec2f_t::zero, vec2f_t::zero, UINT8_MAX, 0, 0, 0, 0.0f, from_nav);
		}

		static void on_focus_lose(ui::input_router_t& router, ui::widget_id_t id, bool from_nav, void* user_data)
		{
			static_cast<world_canvas_events_t*>(user_data)->queue_event(id, canvas_event_type_e::focus_lose, vec2f_t::zero, vec2f_t::zero, UINT8_MAX, 0, 0, 0, 0.0f, from_nav);
		}

		static void on_key(ui::input_router_t& router, ui::widget_id_t id, const ui::key_event_t& event, void* user_data)
		{
			static_cast<world_canvas_events_t*>(user_data)->queue_event(id, canvas_event_type_e::key, vec2f_t::zero, vec2f_t::zero, UINT8_MAX, event.key, event.scan_code, static_cast<u8>(event.action));
		}

		static void on_wheel(ui::input_router_t& router, ui::widget_id_t id, f32 delta, void* user_data)
		{
			static_cast<world_canvas_events_t*>(user_data)->queue_event(id, canvas_event_type_e::wheel, router.get_mouse_position(), vec2f_t::zero, UINT8_MAX, 0, 0, 0, delta);
		}
	};

	bool world_canvas_controller_t::is_recreation_required(const component_canvas_t& config, const component_canvas_t& other)
	{
		return config.text_pool_budget_mb != other.text_pool_budget_mb || config.vertex_pool_budget_mb != other.vertex_pool_budget_mb || config.index_pool_budget_mb != other.index_pool_budget_mb || config.max_widgets != other.max_widgets ||
			   config.is_3d != other.is_3d;
	}

	canvas_widget_handle_t world_canvas_controller_t::make_handle(const component_system_canvas_t& runtime, u16 id)
	{
		const canvas_widget_handle_t generation = runtime.widgets[id].generation;

		return static_cast<canvas_widget_handle_t>(id) | (generation << 16);
	}

	u16 world_canvas_controller_t::resolve_handle(const component_system_canvas_t& runtime, canvas_widget_handle_t handle)
	{
		if (handle == NULL_CANVAS_WIDGET_HANDLE)
			return NULL_WIDGET;

		const ui::widget_id_t id		 = static_cast<ui::widget_id_t>(handle & 0xFFFFu);
		const u16			  generation = static_cast<u16>(handle >> 16);

		if (id >= runtime.config.max_widgets || generation == 0 || runtime.widgets[id].generation != generation || !runtime.context->get_tree().is_alive(id))
			return NULL_WIDGET;

		return id;
	}

	void world_canvas_controller_t::invalidate_recursive(component_system_canvas_t& runtime, u16 id)
	{
		ui::widget_id_t child = runtime.context->get_tree().node(id).first_child;

		while (child != NULL_WIDGET)
		{
			const ui::widget_id_t next = runtime.context->get_tree().node(child).next_sibling;
			invalidate_recursive(runtime, child);
			child = next;
		}

		canvas_widget_state_t& state = runtime.widgets[id];
		state.generation++;

		if (state.generation == 0)
			state.generation = 1;

		state.label = NULL_WIDGET;
		state.kind	= canvas_widget_kind_e::none;
	}

	void world_canvas_controller_t::create_canvas(entity_id_t entity, const component_canvas_t& component)
	{
		ecs_component_table_t&	   system_table = _world->get_component_table(type_id_t<component_system_canvas_t>::value);
		component_system_canvas_t& runtime		= system_table.add_or_get_as<component_system_canvas_t>(entity);

		runtime.context = sfg_new<ui::ui_context>();
		runtime.widgets = static_cast<canvas_widget_state_t*>(SFG_MALLOC(sizeof(canvas_widget_state_t) * component.max_widgets));
		runtime.events	= sfg_new<world_canvas_events_t>();
		runtime.config	= component;
		runtime.entity	= entity;

		if (runtime.widgets == nullptr)
		{
			SFG_ERR("Failed to allocate canvas widget state");
			throw std::bad_alloc{};
		}

		++_widget_generation;

		if (_widget_generation == 0)
			++_widget_generation;

		for (u32 i = 0; i < component.max_widgets; ++i)
			std::construct_at(runtime.widgets + i, canvas_widget_state_t{.generation = _widget_generation});

		runtime.events->widgets = runtime.widgets;
		runtime.events->entity	= entity;
		runtime.events->events.init(static_cast<u32>(component.max_widgets) * 2u);

		SFG_ASSERT(!component.is_3d || (component.canvas_size.x > 0.0f && component.canvas_size.y > 0.0f && component.pixels_per_unit > 0.0f));

		const u32 pipeline_flags = (component.render_stage == canvas_render_stage_e::before_post_process ? shader_variant_flags_canvas_hdr : shader_variant_flags_canvas_ldr) | (component.is_3d ? shader_variant_flags_canvas_3d : 0u);
		const f32 dpi_scale		 = component.is_3d ? 1.0f : _world->get_screen().get_dpi_scale();

		const u32 text_pool_budget_bytes   = static_cast<u32>(component.text_pool_budget_mb * 1024.0f * 1024.0f);
		const u32 vertex_pool_budget_bytes = static_cast<u32>(component.vertex_pool_budget_mb * 1024.0f * 1024.0f);
		const u32 index_pool_budget_bytes  = static_cast<u32>(component.index_pool_budget_mb * 1024.0f * 1024.0f);

		runtime.context->init({
			.canvas =
				{
					.vertex_pool_budget_bytes		= vertex_pool_budget_bytes,
					.index_pool_budget_bytes		= index_pool_budget_bytes,
					.draw_capacity					= static_cast<u32>(component.max_widgets) * 8u,
					.text_cache_vertex_budget_bytes = vertex_pool_budget_bytes,
					.text_cache_index_budget_bytes	= index_pool_budget_bytes,
					.clip_stack_initial_capacity	= math::min<u32>(component.max_widgets, 64u),
					.text_cache_initial_capacity	= component.max_widgets,
				},
			.user_ui_scale			   = component.ui_scale,
			.dpi_scale				   = dpi_scale,
			.max_widgets			   = component.max_widgets,
			.text_pool_budget_bytes	   = text_pool_budget_bytes,
			.snapshot_vertex_max_bytes = vertex_pool_budget_bytes,
			.snapshot_index_max_bytes  = index_pool_budget_bytes,
			.pipeline_variant_flags	   = pipeline_flags,
			.render_snapshots_enabled  = false,
		});

		runtime.context->get_paint().set_pipelines({
			.default_pipeline = component.default_shader,
			.text_pipeline	  = component.text_shader,
			.texture_pipeline = component.texture_shader,
		});
	}

	void world_canvas_controller_t::append_runtime(const component_system_canvas_t& runtime, world_canvas_draw_snapshot_t& destination) const
	{
		ui::vg_canvas_t& canvas			  = runtime.context->get_canvas();
		const u32		 vertex_offset	  = static_cast<u32>(destination.vertices.size());
		const u32		 index_offset	  = static_cast<u32>(destination.indices.size());
		const u32		 parameter_offset = static_cast<u32>(destination.parameters.size());
		const u32		 batch_offset	  = static_cast<u32>(destination.batches.size());
		const u32		 draw_count		  = canvas.get_draw_count();

		if (canvas.get_indices().size == 0)
			return;

		if (canvas.get_vertices().size > destination.vertices.capacity() - vertex_offset)
			return;

		if (canvas.get_indices().size > destination.indices.capacity() - index_offset)
			return;

		if (draw_count > destination.parameters.capacity() - parameter_offset)
			return;

		if (draw_count > destination.batches.capacity() - batch_offset)
			return;

		destination.vertices.resize(vertex_offset + canvas.get_vertices().size);
		destination.indices.resize(index_offset + canvas.get_indices().size);
		destination.parameters.resize(parameter_offset + draw_count);
		destination.batches.resize(batch_offset + draw_count);

		canvas.resolve(runtime.context->get_pipeline_variant_flags());

		const ui::vg_draw_snapshot_t appended =
			canvas.copy_frame(destination.batches.data() + batch_offset, destination.vertices.data() + vertex_offset, destination.indices.data() + index_offset, destination.parameters.data() + parameter_offset, vertex_offset, index_offset, parameter_offset);

		if (runtime.config.is_3d)
		{
			const u32 canvas_index = static_cast<u32>(destination.canvas_data.size());
			destination.canvas_data.push_back({.origin = runtime.config.pivot * runtime.config.canvas_size, .units_per_pixel = 1.0f / runtime.config.pixels_per_unit, .first_parameter = parameter_offset});
			destination.owners.push_back(runtime.entity);

			for (u32 i = 0; i < appended.parameter_count; ++i)
				destination.parameters[parameter_offset + i].canvas_index = canvas_index;
		}

		for (u32 i = 0; i < appended.batch_count; ++i)
			destination.batches[batch_offset + i].is_3d = runtime.config.is_3d;

		destination.indices.resize(index_offset + appended.index_count);
		destination.parameters.resize(parameter_offset + appended.parameter_count);
		destination.batches.resize(batch_offset + appended.batch_count);

		if (batch_offset != 0 && appended.batch_count != 0 && destination.batches[batch_offset - 1].pipeline == destination.batches[batch_offset].pipeline && destination.batches[batch_offset - 1].is_3d == destination.batches[batch_offset].is_3d)
		{
			destination.batches[batch_offset - 1].index_count += destination.batches[batch_offset].index_count;
			destination.batches.erase(destination.batches.begin() + batch_offset);
		}
	}

	void world_canvas_controller_t::destroy_canvas(entity_id_t entity)
	{
		ecs_component_table_t&	   system_table = _world->get_component_table(type_id_t<component_system_canvas_t>::value);
		component_system_canvas_t& runtime		= system_table.get_as<component_system_canvas_t>(entity);

		runtime.context->uninit();
		sfg_delete(runtime.context);
		sfg_delete(runtime.events);
		SFG_FREE(runtime.widgets);
		system_table.remove(entity);
	}

	bool world_canvas_controller_t::map_position(const component_system_canvas_t& runtime, const vec2f_t& position, vec2f_t& mapped, f32& distance) const
	{
		const world_screen_t& screen = _world->get_screen();

		mapped	 = {-1000000.0f, -1000000.0f};
		distance = 0.0f;

		if (!runtime.config.is_3d)
		{
			mapped = screen.screen_to_render_position(position);
			return true;
		}

		vec3f_t near_point = vec3f_t::zero;
		vec3f_t far_point  = vec3f_t::zero;

		if (!screen.screen_to_world(position, 1.0f, near_point) || !screen.screen_to_world(position, 0.5f, far_point))
			return false;

		const component_system_transform_t& transform = _world->get_component_table(type_id_t<component_system_transform_t>::value).get_as_const<component_system_transform_t>(runtime.entity);
		const mat4x3_t						inverse	  = transform.abs_mat.inverse();
		const vec3f_t						origin	  = inverse * near_point;
		const vec3f_t						direction = inverse * far_point - origin;

		if (direction.z <= MATH_EPS || origin.z > 0.0f)
			return false;

		const vec3f_t hit = origin + direction * (-origin.z / direction.z);

		mapped	 = {-hit.x * runtime.config.pixels_per_unit + runtime.config.pivot.x * runtime.config.canvas_size.x, -hit.y * runtime.config.pixels_per_unit + runtime.config.pivot.y * runtime.config.canvas_size.y};
		distance = (transform.abs_mat * hit - near_point).magnitude();
		return true;
	}

	component_system_canvas_t* world_canvas_controller_t::route_pointer(const vec2f_t& position)
	{
		const ecs_component_table_ref_t refs[]			= {_world->get_component_table(type_id_t<component_system_canvas_t>::value).ref()};
		component_system_canvas_t*		target			= nullptr;
		f32								target_distance = 0.0f;

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
		{
			component_system_canvas_t& runtime	= row.get_mutable<component_system_canvas_t>(0);
			vec2f_t					   mapped	= vec2f_t::zero;
			f32						   distance = 0.0f;

			if (!runtime.config.input_enabled || !map_position(runtime, position, mapped, distance) || runtime.context->get_input().get_hit_widget(mapped) == NULL_WIDGET)
				continue;

			if (runtime.config.is_3d && _world->get_physics().is_init())
			{
				vec3f_t near_point = vec3f_t::zero;
				vec3f_t far_point  = vec3f_t::zero;

				_world->get_screen().screen_to_world(position, 1.0f, near_point);
				_world->get_screen().screen_to_world(position, 0.5f, far_point);

				physics_hit_t				 hit = {};
				const physics_query_filter_t filter{.ignored_entity = runtime.entity, .flags = physics_query_flag_static | physics_query_flag_kinematic | physics_query_flag_dynamic};

				if (_world->get_physics().raycast_closest({.origin = near_point, .direction = far_point - near_point, .distance = distance}, hit, filter) && hit.distance < distance - MATH_EPS)
					continue;
			}

			const bool same_space = target != nullptr && runtime.config.is_3d == target->config.is_3d;
			const bool nearer	  = same_space && runtime.config.is_3d && distance < target_distance;
			const bool ordered	  = same_space && (!runtime.config.is_3d || distance == target_distance) && runtime.config.sort_order >= target->config.sort_order;

			if (target == nullptr || (!runtime.config.is_3d && target->config.is_3d) || nearer || ordered)
			{
				target			= &runtime;
				target_distance = distance;
			}
		}

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
		{
			component_system_canvas_t& runtime	= row.get_mutable<component_system_canvas_t>(0);
			bool					   captured = false;

			for (u8 button = 0; button < static_cast<u8>(ui::mouse_button_e::count); ++button)
				captured |= runtime.context->get_input().is_pressed(static_cast<ui::mouse_button_e>(button)) != NULL_WIDGET;

			vec2f_t mapped	 = {-1000000.0f, -1000000.0f};
			f32		distance = 0.0f;

			if (&runtime == target || captured)
				map_position(runtime, position, mapped, distance);

			runtime.context->on_mouse_move(mapped, &runtime == target);
		}

		return target;
	}

	void world_canvas_controller_t::init(world_t& world)
	{
		_world = &world;
	}

	void world_canvas_controller_t::uninit()
	{
		clear();
		_world = nullptr;
	}

	void world_canvas_controller_t::clear()
	{
		const ecs_component_table_ref_t refs[]	 = {_world->get_component_table(type_id_t<component_system_canvas_t>::value).ref()};
		frame_vector_t<entity_id_t>		entities = {};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
			entities.push_back(row.id);

		for (const entity_id_t entity : entities)
			destroy_canvas(entity);
	}

	void world_canvas_controller_t::clear_widgets()
	{
		const ecs_component_table_ref_t refs[] = {_world->get_component_table(type_id_t<component_system_canvas_t>::value).ref()};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
			clear_widgets(row.id);
	}

	void world_canvas_controller_t::destroy_entity(entity_id_t entity)
	{
		const ecs_component_table_t& system_table = _world->get_component_table(type_id_t<component_system_canvas_t>::value);

		if (!system_table.has(entity))
			return;

		destroy_canvas(entity);
	}

	void world_canvas_controller_t::sync_create_destroy_canvases()
	{
		ecs_component_table_t&		 system_table	= _world->get_component_table(type_id_t<component_system_canvas_t>::value);
		const ecs_component_table_t& canvas_table	= _world->get_component_table(type_id_t<component_canvas_t>::value);
		const ecs_component_table_t& disabled_table = _world->get_component_table(type_id_t<component_disabled_t>::value);

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
				component_system_canvas_t& runtime	 = row.get_mutable<component_system_canvas_t>(0);
				const component_canvas_t&  component = row.get<component_canvas_t>(1);

				if (is_recreation_required(runtime.config, component))
					destroy_entities.push_back(row.id);
				else
				{
					SFG_ASSERT(!component.is_3d || (component.canvas_size.x > 0.0f && component.canvas_size.y > 0.0f && component.pixels_per_unit > 0.0f));

					if (runtime.config.ui_scale != component.ui_scale)
						runtime.context->set_user_ui_scale(component.ui_scale);

					if (runtime.config.render_stage != component.render_stage)
					{
						const u32 pipeline_flags = (component.render_stage == canvas_render_stage_e::before_post_process ? shader_variant_flags_canvas_hdr : shader_variant_flags_canvas_ldr) | (component.is_3d ? shader_variant_flags_canvas_3d : 0u);

						runtime.context->set_pipeline_variant_flags(pipeline_flags);
					}

					if (runtime.config.default_shader != component.default_shader || runtime.config.text_shader != component.text_shader || runtime.config.texture_shader != component.texture_shader)
					{
						ui::paint_layer_t& paint = runtime.context->get_paint();

						for (u16 id = 0; id < component.max_widgets; ++id)
						{
							const canvas_widget_kind_e kind = runtime.widgets[id].kind;

							if (kind == canvas_widget_kind_e::none)
								continue;

							const resource_handle_t previous_pipeline = kind == canvas_widget_kind_e::text ? runtime.config.text_shader : kind == canvas_widget_kind_e::image ? runtime.config.texture_shader : runtime.config.default_shader;
							ui::paint_def_t&		def				  = paint.def(id);

							if (def.render_state.pipeline == previous_pipeline)
								def.render_state.pipeline = kind == canvas_widget_kind_e::text ? component.text_shader : kind == canvas_widget_kind_e::image ? component.texture_shader : component.default_shader;
						}

						paint.set_pipelines({
							.default_pipeline = component.default_shader,
							.text_pipeline	  = component.text_shader,
							.texture_pipeline = component.texture_shader,
						});
					}

					runtime.config = component;
				}
			}
		}

		std::sort(destroy_entities.begin(), destroy_entities.end());
		destroy_entities.erase(std::unique(destroy_entities.begin(), destroy_entities.end()), destroy_entities.end());

		for (const entity_id_t entity : destroy_entities)
			destroy_canvas(entity);

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
			const component_canvas_t& component = canvas_table.get_as_const<component_canvas_t>(entity);
			create_canvas(entity, component);
		}
	}

	void world_canvas_controller_t::tick(f32 dt_seconds)
	{
		sync_create_destroy_canvases();

		const world_screen_t&			screen		= _world->get_screen();
		const vec2u16_t					render_size = screen.get_render_size();
		const vec4f_t					screen_rect = {0.0f, 0.0f, static_cast<f32>(render_size.x), static_cast<f32>(render_size.y)};
		const ecs_component_table_ref_t refs[]		= {_world->get_component_table(type_id_t<component_system_canvas_t>::value).ref()};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
		{
			component_system_canvas_t& runtime = row.get_mutable<component_system_canvas_t>(0);
			const bool				   is_3d   = runtime.config.is_3d;
			const vec4f_t			   bounds  = is_3d ? vec4f_t(0.0f, 0.0f, runtime.config.canvas_size.x, runtime.config.canvas_size.y) : screen_rect;

			runtime.context->tick(bounds, is_3d ? 1.0f : screen.get_dpi_scale(), dt_seconds);
		}
	}

	void world_canvas_controller_t::write_render_snapshot(world_canvas_render_snapshot_t& snapshot) const
	{
		snapshot.clear();

		const ecs_component_table_ref_t					 refs[] = {_world->get_component_table(type_id_t<component_system_canvas_t>::value).ref()};
		frame_vector_t<const component_system_canvas_t*> sorted = {};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
			sorted.push_back(&row.get<component_system_canvas_t>(0));

		std::stable_sort(sorted.begin(), sorted.end(), [](const component_system_canvas_t* left, const component_system_canvas_t* right) { return left->config.sort_order < right->config.sort_order; });

		for (const component_system_canvas_t* runtime : sorted)
		{
			if (runtime->config.is_3d)
			{
				const size_t canvas_count = snapshot.before_post_process.canvas_data.size() + snapshot.after_post_process.canvas_data.size();

				if (canvas_count == snapshot.canvas_max)
					continue;
			}

			world_canvas_draw_snapshot_t& destination = runtime->config.render_stage == canvas_render_stage_e::before_post_process ? snapshot.before_post_process : snapshot.after_post_process;

			append_runtime(*runtime, destination);
		}
	}

	bool world_canvas_controller_t::key_event(u16 key, u16 scan_code, u8 action)
	{
		const ecs_component_table_ref_t refs[] = {_world->get_component_table(type_id_t<component_system_canvas_t>::value).ref()};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
		{
			component_system_canvas_t& runtime = row.get_mutable<component_system_canvas_t>(0);

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

		const ui::mouse_button_e		mapped_button = static_cast<ui::mouse_button_e>(button);
		component_system_canvas_t*		target		  = route_pointer(position);
		const ecs_component_table_ref_t refs[]		  = {_world->get_component_table(type_id_t<component_system_canvas_t>::value).ref()};

		if (action != 0)
		{
			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
			{
				component_system_canvas_t& runtime = row.get_mutable<component_system_canvas_t>(0);

				if (runtime.context->get_input().is_pressed(mapped_button) != NULL_WIDGET)
				{
					runtime.context->on_mouse_button(mapped_button, false);
					return true;
				}
			}

			return false;
		}

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
		{
			component_system_canvas_t& runtime = row.get_mutable<component_system_canvas_t>(0);

			if (&runtime != target)
				runtime.context->get_input().set_focus(NULL_WIDGET, false);
		}

		if (target == nullptr)
			return false;

		target->context->on_mouse_button(mapped_button, true);
		return true;
	}

	bool world_canvas_controller_t::mouse_move_event(const vec2f_t& position)
	{
		if (route_pointer(position) != nullptr)
			return true;

		const ecs_component_table_ref_t refs[] = {_world->get_component_table(type_id_t<component_system_canvas_t>::value).ref()};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
		{
			const ui::input_router_t& input = row.get<component_system_canvas_t>(0).context->get_input();

			for (u8 button = 0; button < static_cast<u8>(ui::mouse_button_e::count); ++button)
			{
				if (input.is_pressed(static_cast<ui::mouse_button_e>(button)) != NULL_WIDGET)
					return true;
			}
		}

		return false;
	}

	bool world_canvas_controller_t::mouse_wheel_event(const vec2f_t& position, f32 delta)
	{
		component_system_canvas_t* target = route_pointer(position);

		if (target == nullptr)
			return false;

		target->context->on_wheel(delta);
		return true;
	}

	bool world_canvas_controller_t::is_keyboard_focus_active() const
	{
		const ecs_component_table_ref_t refs[] = {_world->get_component_table(type_id_t<component_system_canvas_t>::value).ref()};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
		{
			const component_system_canvas_t& runtime = row.get_mutable<component_system_canvas_t>(0);

			if (runtime.config.input_enabled && runtime.context->get_input().get_focused() != NULL_WIDGET)
				return true;
		}

		return false;
	}

	void world_canvas_controller_t::dispatch_events()
	{
		const ecs_component_table_ref_t refs[]	= {_world->get_component_table(type_id_t<component_system_canvas_t>::value).ref()};
		frame_vector_t<canvas_event_t>	pending = {};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
		{
			world_canvas_events_t& queue = *row.get<component_system_canvas_t>(0).events;

			pending.insert(pending.end(), queue.events.begin(), queue.events.end());
			queue.events.resize(0);
		}

		for (const canvas_event_t& event : pending)
		{
			const component_system_canvas_t* runtime = _world->get_component_table(type_id_t<component_system_canvas_t>::value).find_as_const<component_system_canvas_t>(event.canvas);

			if (_world->is_playing() && runtime != nullptr && resolve_handle(*runtime, event.widget) != NULL_WIDGET)
				game_plugin_runtime_t::get().canvas_event(*_world, event);
		}
	}

	ui::ui_context* world_canvas_controller_t::get_ui_context(entity_id_t canvas)
	{
		component_system_canvas_t* runtime = _world->get_component_table(type_id_t<component_system_canvas_t>::value).find_as<component_system_canvas_t>(canvas);

		if (runtime == nullptr)
		{
			SFG_ERR("canvas entity {0} has no runtime canvas", canvas);
			return nullptr;
		}

		return runtime->context;
	}

	canvas_widget_handle_t world_canvas_controller_t::create_frame(entity_id_t canvas, canvas_widget_handle_t parent, const ui::layout_in_t& layout, const ui::vg_rect_paint_t& style)
	{
		component_system_canvas_t* runtime = _world->get_component_table(type_id_t<component_system_canvas_t>::value).find_as<component_system_canvas_t>(canvas);

		if (runtime == nullptr)
		{
			SFG_ERR("canvas entity {0} has no runtime canvas", canvas);
			return NULL_CANVAS_WIDGET_HANDLE;
		}

		const ui::widget_id_t parent_id = parent == NULL_CANVAS_WIDGET_HANDLE ? runtime->context->get_root() : resolve_handle(*runtime, parent);

		if (parent_id == NULL_WIDGET)
		{
			SFG_ERR("invalid parent widget handle {0} for canvas entity {1}", parent, canvas);
			return NULL_CANVAS_WIDGET_HANDLE;
		}

		const ui::widget_id_t id = runtime->context->allocate_widget();

		runtime->widgets[id].kind = canvas_widget_kind_e::frame;
		runtime->context->get_tree().attach(parent_id, id);
		runtime->context->get_tree().in(id) = layout;
		runtime->context->get_tree().mark_layout_dirty();
		runtime->context->get_paint().set_rect(id, style);
		return make_handle(*runtime, id);
	}

	canvas_widget_handle_t world_canvas_controller_t::create_text(entity_id_t canvas, canvas_widget_handle_t parent, const ui::layout_in_t& layout, const char* text, const ui::vg_text_style_t& style)
	{
		component_system_canvas_t* runtime = _world->get_component_table(type_id_t<component_system_canvas_t>::value).find_as<component_system_canvas_t>(canvas);

		if (runtime == nullptr)
		{
			SFG_ERR("canvas entity {0} has no runtime canvas", canvas);
			return NULL_CANVAS_WIDGET_HANDLE;
		}

		const ui::widget_id_t parent_id = parent == NULL_CANVAS_WIDGET_HANDLE ? runtime->context->get_root() : resolve_handle(*runtime, parent);

		if (parent_id == NULL_WIDGET)
		{
			SFG_ERR("invalid parent widget handle {0} for canvas entity {1}", parent, canvas);
			return NULL_CANVAS_WIDGET_HANDLE;
		}

		const ui::widget_id_t id = runtime->context->allocate_widget();

		runtime->widgets[id].kind = canvas_widget_kind_e::text;
		runtime->context->get_tree().attach(parent_id, id);
		runtime->context->get_tree().in(id) = layout;
		runtime->context->get_tree().mark_layout_dirty();
		runtime->context->set_widget_text(id, text);

		ui::vg_text_style_t resolved_style = style;

		if (resolved_style.font == NULL_RESOURCE_HANDLE)
			resolved_style.font = runtime->config.default_font;

		runtime->context->get_paint().set_text(id, runtime->context->widget_text(id), runtime->context->widget_text_len(id), resolved_style);
		return make_handle(*runtime, id);
	}

	canvas_widget_handle_t world_canvas_controller_t::create_image(entity_id_t canvas, canvas_widget_handle_t parent, const ui::layout_in_t& layout, resource_handle_t texture, const vec4f_t& tint)
	{
		component_system_canvas_t* runtime = _world->get_component_table(type_id_t<component_system_canvas_t>::value).find_as<component_system_canvas_t>(canvas);

		if (runtime == nullptr)
		{
			SFG_ERR("canvas entity {0} has no runtime canvas", canvas);
			return NULL_CANVAS_WIDGET_HANDLE;
		}

		const ui::widget_id_t parent_id = parent == NULL_CANVAS_WIDGET_HANDLE ? runtime->context->get_root() : resolve_handle(*runtime, parent);

		if (parent_id == NULL_WIDGET)
		{
			SFG_ERR("invalid parent widget handle {0} for canvas entity {1}", parent, canvas);
			return NULL_CANVAS_WIDGET_HANDLE;
		}

		const ui::widget_id_t id = runtime->context->allocate_widget();

		runtime->widgets[id].kind = canvas_widget_kind_e::image;
		runtime->context->get_tree().attach(parent_id, id);
		runtime->context->get_tree().in(id) = layout;
		runtime->context->get_tree().mark_layout_dirty();
		set_image(canvas, make_handle(*runtime, id), texture, tint);
		return make_handle(*runtime, id);
	}

	canvas_widget_handle_t world_canvas_controller_t::create_button(
		entity_id_t canvas, canvas_widget_handle_t parent, const ui::layout_in_t& layout, const char* text, const ui::vg_rect_paint_t& frame_style, const ui::vg_text_style_t& text_style, const vec4f_t& hover_color, const vec4f_t& press_color)
	{
		component_system_canvas_t* runtime = _world->get_component_table(type_id_t<component_system_canvas_t>::value).find_as<component_system_canvas_t>(canvas);

		if (runtime == nullptr)
		{
			SFG_ERR("canvas entity {0} has no runtime canvas", canvas);
			return NULL_CANVAS_WIDGET_HANDLE;
		}

		const ui::widget_id_t parent_id = parent == NULL_CANVAS_WIDGET_HANDLE ? runtime->context->get_root() : resolve_handle(*runtime, parent);

		if (parent_id == NULL_WIDGET)
		{
			SFG_ERR("invalid parent widget handle {0} for canvas entity {1}", parent, canvas);
			return NULL_CANVAS_WIDGET_HANDLE;
		}

		const ui::widget_id_t id = runtime->context->allocate_widget();

		runtime->widgets[id].kind = canvas_widget_kind_e::button;
		runtime->context->get_tree().attach(parent_id, id);
		runtime->context->get_tree().in(id) = layout;
		runtime->context->get_tree().in(id).flags |= ui::wf_input | ui::wf_focusable;
		runtime->context->get_tree().mark_layout_dirty();
		runtime->context->get_paint().set_rect(id, frame_style);
		runtime->context->get_paint().set_hover_color(id, hover_color);
		runtime->context->get_paint().set_press_color(id, press_color);

		const ui::listener_bundle_t listener = {
			.on_press		 = world_canvas_events_t::on_press,
			.on_release		 = world_canvas_events_t::on_release,
			.on_click		 = world_canvas_events_t::on_click,
			.on_double_click = world_canvas_events_t::on_double_click,
			.on_hover_enter	 = world_canvas_events_t::on_hover_enter,
			.on_hover_exit	 = world_canvas_events_t::on_hover_exit,
			.on_hover_move	 = world_canvas_events_t::on_hover_move,
			.on_drag_begin	 = world_canvas_events_t::on_drag_begin,
			.on_drag		 = world_canvas_events_t::on_drag,
			.on_drag_end	 = world_canvas_events_t::on_drag_end,
			.on_focus_gain	 = world_canvas_events_t::on_focus_gain,
			.on_focus_lose	 = world_canvas_events_t::on_focus_lose,
			.on_key			 = world_canvas_events_t::on_key,
			.on_wheel		 = world_canvas_events_t::on_wheel,
			.user_data		 = runtime->events,
		};

		runtime->context->get_input().set_listener(id, listener);

		const ui::widget_id_t label = runtime->context->allocate_widget();

		runtime->widgets[label].kind = canvas_widget_kind_e::text;
		runtime->widgets[id].label	 = label;
		runtime->context->get_tree().attach(id, label);
		runtime->context->get_tree().draw_order(label) = runtime->context->get_tree().draw_order_const(id);

		ui::layout_in_t& label_layout = runtime->context->get_tree().in(label);

		label_layout.pos_mode_x = ui::pos_mode_e::relative_in_parent;
		label_layout.pos_mode_y = ui::pos_mode_e::relative_in_parent;
		label_layout.pos_value	= {0.5f, 0.5f};
		label_layout.anchor_x	= ui::anchor_e::center;
		label_layout.anchor_y	= ui::anchor_e::center;
		runtime->context->set_widget_text(label, text);

		ui::vg_text_style_t resolved_style = text_style;

		if (resolved_style.font == NULL_RESOURCE_HANDLE)
			resolved_style.font = runtime->config.default_font;

		runtime->context->get_paint().set_text(label, runtime->context->widget_text(label), runtime->context->widget_text_len(label), resolved_style);
		return make_handle(*runtime, id);
	}

	bool world_canvas_controller_t::destroy_widget(entity_id_t canvas, canvas_widget_handle_t widget)
	{
		component_system_canvas_t* runtime = _world->get_component_table(type_id_t<component_system_canvas_t>::value).find_as<component_system_canvas_t>(canvas);

		if (runtime == nullptr)
		{
			SFG_ERR("canvas entity {0} has no runtime canvas", canvas);
			return false;
		}

		const ui::widget_id_t id = resolve_handle(*runtime, widget);

		if (id == NULL_WIDGET)
		{
			SFG_ERR("invalid widget handle {0} for canvas entity {1}", widget, canvas);
			return false;
		}

		invalidate_recursive(*runtime, id);
		runtime->context->deallocate_widget(id);
		return true;
	}

	bool world_canvas_controller_t::clear_widgets(entity_id_t canvas)
	{
		component_system_canvas_t* runtime = _world->get_component_table(type_id_t<component_system_canvas_t>::value).find_as<component_system_canvas_t>(canvas);

		if (runtime == nullptr)
		{
			SFG_ERR("canvas entity {0} has no runtime canvas", canvas);
			return false;
		}

		const ui::widget_id_t root	= runtime->context->get_root();
		ui::widget_id_t		  child = runtime->context->get_tree().node(root).first_child;

		while (child != NULL_WIDGET)
		{
			const ui::widget_id_t next = runtime->context->get_tree().node(child).next_sibling;

			invalidate_recursive(*runtime, child);
			runtime->context->deallocate_widget(child);
			child = next;
		}

		return true;
	}

	bool world_canvas_controller_t::set_layout(entity_id_t canvas, canvas_widget_handle_t widget, const ui::layout_in_t& layout)
	{
		component_system_canvas_t* runtime = _world->get_component_table(type_id_t<component_system_canvas_t>::value).find_as<component_system_canvas_t>(canvas);

		if (runtime == nullptr)
		{
			SFG_ERR("canvas entity {0} has no runtime canvas", canvas);
			return false;
		}

		const ui::widget_id_t id = resolve_handle(*runtime, widget);

		if (id == NULL_WIDGET)
		{
			SFG_ERR("invalid widget handle {0} for canvas entity {1}", widget, canvas);
			return false;
		}

		runtime->context->get_tree().in(id) = layout;
		runtime->context->get_tree().mark_layout_dirty();
		return true;
	}

	bool world_canvas_controller_t::set_visible(entity_id_t canvas, canvas_widget_handle_t widget, bool visible)
	{
		component_system_canvas_t* runtime = _world->get_component_table(type_id_t<component_system_canvas_t>::value).find_as<component_system_canvas_t>(canvas);

		if (runtime == nullptr)
		{
			SFG_ERR("canvas entity {0} has no runtime canvas", canvas);
			return false;
		}

		const ui::widget_id_t id = resolve_handle(*runtime, widget);

		if (id == NULL_WIDGET)
		{
			SFG_ERR("invalid widget handle {0} for canvas entity {1}", widget, canvas);
			return false;
		}

		runtime->context->get_tree().set_visible(id, visible);
		return true;
	}

	bool world_canvas_controller_t::set_enabled(entity_id_t canvas, canvas_widget_handle_t widget, bool enabled)
	{
		component_system_canvas_t* runtime = _world->get_component_table(type_id_t<component_system_canvas_t>::value).find_as<component_system_canvas_t>(canvas);

		if (runtime == nullptr)
		{
			SFG_ERR("canvas entity {0} has no runtime canvas", canvas);
			return false;
		}

		const ui::widget_id_t id = resolve_handle(*runtime, widget);

		if (id == NULL_WIDGET)
		{
			SFG_ERR("invalid widget handle {0} for canvas entity {1}", widget, canvas);
			return false;
		}

		ui::layout_in_t& layout = runtime->context->get_tree().in(id);

		if (enabled)
			layout.flags &= ~ui::wf_disabled;
		else
			layout.flags |= ui::wf_disabled;

		return true;
	}

	bool world_canvas_controller_t::set_text(entity_id_t canvas, canvas_widget_handle_t widget, const char* text)
	{
		component_system_canvas_t* runtime = _world->get_component_table(type_id_t<component_system_canvas_t>::value).find_as<component_system_canvas_t>(canvas);

		if (runtime == nullptr)
		{
			SFG_ERR("canvas entity {0} has no runtime canvas", canvas);
			return false;
		}

		ui::widget_id_t id = resolve_handle(*runtime, widget);

		if (id == NULL_WIDGET)
		{
			SFG_ERR("invalid widget handle {0} for canvas entity {1}", widget, canvas);
			return false;
		}

		if (runtime->widgets[id].kind == canvas_widget_kind_e::button)
			id = runtime->widgets[id].label;

		if (runtime->widgets[id].kind != canvas_widget_kind_e::text)
		{
			SFG_ERR("widget handle {0} on canvas entity {1} is not text or a button label", widget, canvas);
			return false;
		}

		runtime->context->set_widget_text(id, text);
		return true;
	}

	bool world_canvas_controller_t::set_frame_style(entity_id_t canvas, canvas_widget_handle_t widget, const ui::vg_rect_paint_t& style)
	{
		component_system_canvas_t* runtime = _world->get_component_table(type_id_t<component_system_canvas_t>::value).find_as<component_system_canvas_t>(canvas);

		if (runtime == nullptr)
		{
			SFG_ERR("canvas entity {0} has no runtime canvas", canvas);
			return false;
		}

		const ui::widget_id_t id = resolve_handle(*runtime, widget);

		if (id == NULL_WIDGET)
		{
			SFG_ERR("invalid widget handle {0} for canvas entity {1}", widget, canvas);
			return false;
		}

		if (runtime->widgets[id].kind != canvas_widget_kind_e::frame && runtime->widgets[id].kind != canvas_widget_kind_e::button)
		{
			SFG_ERR("widget handle {0} on canvas entity {1} is not a frame or button", widget, canvas);
			return false;
		}

		runtime->context->get_paint().set_rect(id, style);
		return true;
	}

	bool world_canvas_controller_t::set_text_style(entity_id_t canvas, canvas_widget_handle_t widget, const ui::vg_text_style_t& style)
	{
		component_system_canvas_t* runtime = _world->get_component_table(type_id_t<component_system_canvas_t>::value).find_as<component_system_canvas_t>(canvas);

		if (runtime == nullptr)
		{
			SFG_ERR("canvas entity {0} has no runtime canvas", canvas);
			return false;
		}

		ui::widget_id_t id = resolve_handle(*runtime, widget);

		if (id == NULL_WIDGET)
		{
			SFG_ERR("invalid widget handle {0} for canvas entity {1}", widget, canvas);
			return false;
		}

		if (runtime->widgets[id].kind == canvas_widget_kind_e::button)
			id = runtime->widgets[id].label;

		if (runtime->widgets[id].kind != canvas_widget_kind_e::text)
		{
			SFG_ERR("widget handle {0} on canvas entity {1} is not text or a button label", widget, canvas);
			return false;
		}

		ui::vg_text_style_t resolved_style = style;

		if (resolved_style.font == NULL_RESOURCE_HANDLE)
			resolved_style.font = runtime->config.default_font;

		runtime->context->get_paint().set_text(id, runtime->context->widget_text(id), runtime->context->widget_text_len(id), resolved_style);
		return true;
	}

	bool world_canvas_controller_t::set_image(entity_id_t canvas, canvas_widget_handle_t widget, resource_handle_t texture, const vec4f_t& tint)
	{
		component_system_canvas_t* runtime = _world->get_component_table(type_id_t<component_system_canvas_t>::value).find_as<component_system_canvas_t>(canvas);

		if (runtime == nullptr)
		{
			SFG_ERR("canvas entity {0} has no runtime canvas", canvas);
			return false;
		}

		const ui::widget_id_t id = resolve_handle(*runtime, widget);

		if (id == NULL_WIDGET)
		{
			SFG_ERR("invalid widget handle {0} for canvas entity {1}", widget, canvas);
			return false;
		}

		if (runtime->widgets[id].kind != canvas_widget_kind_e::image)
		{
			SFG_ERR("widget handle {0} on canvas entity {1} is not an image", widget, canvas);
			return false;
		}

		ui::vg_rect_paint_t rect = {};

		rect.fill_color_a = tint;
		rect.fill_color_b = tint;

		ui::ui_render_state_t state = {};

		state.pipeline			  = runtime->config.texture_shader;
		state.constants[0].handle = texture;
		state.constants[0].type	  = ui::ui_resource_type_e::texture;
		runtime->context->get_paint().set_rect(id, rect, state);
		return true;
	}

}
