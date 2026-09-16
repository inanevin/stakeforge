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

#include "world/editor_world.hpp"
#include "world/editor_world_camera_fly.hpp"
#include "world/editor_world_camera_orbit.hpp"
#include "world/editor_world_rendering.hpp"
#include "world/editor_world_util.hpp"
#include "ui/panels/editor_theme.hpp"
#include <sfg/data/frame_vector.hpp>
#include <sfg/data/istream.hpp>
#include <sfg/math/aabb.hpp>
#include <sfg/math/color.hpp>
#include <sfg/math/math.hpp>
#include <sfg/math/mat4x3.hpp>
#include <sfg/runtime/physics/physics_world.hpp>
#include <sfg/runtime/render/render_view.hpp>
#include <sfg/runtime/render/world_render_view.hpp>
#include <sfg/runtime/render/world_rendering.hpp>
#include <sfg/runtime/resources/common_resources.hpp>
#include <sfg/runtime/resources/world_cook.hpp>
#include <sfg/runtime/world/world_init_config.hpp>
#include <sfg/runtime/world/world_debug_draw.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/runtime/engine/engine_runtime.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>
#include <sfg/runtime/world/ecs.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/system_components.hpp>
#include <sfg/runtime/world/world_snapshot_producer.hpp>
#include <sfg/runtime/world/world_util.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
#define EDITOR_WORLD_SNAPSHOT_SLOT_COUNT				3
#define EDITOR_WORLD_SNAPSHOT_SLOT_MASK					0x3
#define EDITOR_WORLD_SNAPSHOT_FRESH_FLAG				0x80
#define EDITOR_WORLD_PICK_RESULT_REQUEST_SHIFT			32
#define EDITOR_WORLD_DEBUG_LINE_VERTEX_MAX_COUNT		(8192 * 4)
#define EDITOR_WORLD_DEBUG_LINE_INDEX_MAX_COUNT			(8192 * 6)
#define EDITOR_WORLD_DEBUG_TRIANGLE_VERTEX_MAX_COUNT	164000
#define EDITOR_WORLD_DEBUG_TRIANGLE_INDEX_MAX_COUNT		164000
#define EDITOR_WORLD_DEBUG_TEXT_COMMAND_MAX_COUNT		256
#define EDITOR_WORLD_DEBUG_TEXT_BUDGET_BYTES			32768
#define EDITOR_WORLD_DEBUG_TEXT_VERTEX_MAX_COUNT		16384
#define EDITOR_WORLD_DEBUG_TEXT_INDEX_MAX_COUNT			24576
#define EDITOR_WORLD_DEBUG_TEXTURE_MAX_COUNT			8192
#define EDITOR_MAIN_WORLD_LIGHT_MAX_COUNT				1024
#define EDITOR_MAIN_WORLD_REFLECTION_PROBE_MAX_COUNT	256
#define EDITOR_PREVIEW_WORLD_LIGHT_MAX_COUNT			16
#define EDITOR_PREVIEW_WORLD_REFLECTION_PROBE_MAX_COUNT 8

	editor_world_init_config_t editor_world_init_config_t::make_main(vec2u16_t render_resolution)
	{
		physics_runtime_config_t physics_config = {};

		physics_config.physics_enabled = true;

		return {
			.physics = physics_config,
			.render_context =
				{
					.size				  = render_resolution,
					.entity_max			  = 1024 * 10,
					.sprite_max			  = 256,
					.particle_max		  = 8192,
					.bone_max			  = 4096,
					.light_max			  = EDITOR_MAIN_WORLD_LIGHT_MAX_COUNT,
					.reflection_probe_max = EDITOR_MAIN_WORLD_REFLECTION_PROBE_MAX_COUNT,
					.shadow_view_max	  = ENGINE_SHADOW_VIEW_MAX,
				},
			.debug_draw =
				{
					.line_vertex_max_count	   = EDITOR_WORLD_DEBUG_LINE_VERTEX_MAX_COUNT,
					.line_index_max_count	   = EDITOR_WORLD_DEBUG_LINE_INDEX_MAX_COUNT,
					.triangle_vertex_max_count = EDITOR_WORLD_DEBUG_TRIANGLE_VERTEX_MAX_COUNT,
					.triangle_index_max_count  = EDITOR_WORLD_DEBUG_TRIANGLE_INDEX_MAX_COUNT,
					.text_command_max_count	   = EDITOR_WORLD_DEBUG_TEXT_COMMAND_MAX_COUNT,
					.text_budget_bytes		   = EDITOR_WORLD_DEBUG_TEXT_BUDGET_BYTES,
					.text_vertex_max_count	   = EDITOR_WORLD_DEBUG_TEXT_VERTEX_MAX_COUNT,
					.text_index_max_count	   = EDITOR_WORLD_DEBUG_TEXT_INDEX_MAX_COUNT,
					.texture_max_count		   = EDITOR_WORLD_DEBUG_TEXTURE_MAX_COUNT,
				},
			.world							  = {.render_resolution = render_resolution},
			.selected_entity_initial_capacity = 256,
		};
	}

	editor_world_init_config_t editor_world_init_config_t::make_preview(vec2u16_t render_resolution)
	{
		return {
			.render_context =
				{
					.size				  = render_resolution,
					.entity_max			  = 128,
					.sprite_max			  = 16,
					.particle_max		  = 0,
					.bone_max			  = MAX_SKELETON_BONES,
					.light_max			  = EDITOR_PREVIEW_WORLD_LIGHT_MAX_COUNT,
					.reflection_probe_max = EDITOR_PREVIEW_WORLD_REFLECTION_PROBE_MAX_COUNT,
					.shadow_view_max	  = 8,
				},
			.debug_draw =
				{
					.line_vertex_max_count	   = EDITOR_WORLD_DEBUG_LINE_VERTEX_MAX_COUNT,
					.line_index_max_count	   = EDITOR_WORLD_DEBUG_LINE_INDEX_MAX_COUNT,
					.triangle_vertex_max_count = EDITOR_WORLD_DEBUG_TRIANGLE_VERTEX_MAX_COUNT,
					.triangle_index_max_count  = EDITOR_WORLD_DEBUG_TRIANGLE_INDEX_MAX_COUNT,
					.text_command_max_count	   = EDITOR_WORLD_DEBUG_TEXT_COMMAND_MAX_COUNT,
					.text_budget_bytes		   = EDITOR_WORLD_DEBUG_TEXT_BUDGET_BYTES,
					.text_vertex_max_count	   = EDITOR_WORLD_DEBUG_TEXT_VERTEX_MAX_COUNT,
					.text_index_max_count	   = EDITOR_WORLD_DEBUG_TEXT_INDEX_MAX_COUNT,
					.texture_max_count		   = EDITOR_WORLD_DEBUG_TEXTURE_MAX_COUNT,
				},
			.particle_simulation			  = {.page_size = 4 * 1024},
			.world							  = {.render_resolution = render_resolution},
			.selected_entity_initial_capacity = 64,
		};
	}

	void editor_world_t::init(const editor_world_init_config_t& init_config, editor_world_handle_t handle, editor_world_edit_type_e edit_type, editor_world_tick_callback_t tick_callback, void* tick_callback_user_data)
	{
		SFG_ASSERT(init_config.world.render_resolution == init_config.render_context.size);

		_world.init(init_config.world, init_config.debug_draw, init_config.physics, init_config.particle_simulation);
		_edit_context.init(edit_type);
		_edit_context.set_world(handle);
		_input_controller.init(*this, handle);
		_entity_gizmo.init(_world, _edit_context);
		_gizmo_callbacks		 = _entity_gizmo.get_callbacks();
		_custom_gizmo			 = false;
		_tick_callback			 = tick_callback;
		_tick_callback_user_data = tick_callback_user_data;

		_producer_slot		  = 0;
		_consumer_slot		  = 1;
		_latest_snapshot_slot = UINT8_MAX;
		_snapshot_mailbox.store(2, std::memory_order_relaxed);
		_pick_result.store(0, std::memory_order_relaxed);
		_pending_pick_request		 = {};
		_last_render_pick_request_id = 0;
		_next_pick_request_id		 = 0;
		_view_rotation				 = quat_t::identity;
		_camera_type				 = editor_world_camera_type_e::fly;
		_play_mode					 = editor_play_mode_e::none;
		_play_snapshot.shrink(0);

		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
			_object_id_readback_valid[i] = false;

		_render_resolution = init_config.render_context.size;
		_render_context.init(init_config.render_context, init_config.debug_draw);

		for (u32 i = 0; i < EDITOR_WORLD_SNAPSHOT_SLOT_COUNT; ++i)
		{
			_snapshot_slots[i].reserve();

			editor_world_snapshot_data_t* data = new editor_world_snapshot_data_t();

			data->selected_entities.reserve(init_config.selected_entity_initial_capacity);
			_snapshot_slots[i].user_data = data;
		}
	}

	void editor_world_t::uninit()
	{
		_input_controller.uninit();

		if (_play_mode != editor_play_mode_e::none)
		{
			_world.end_play();
			_play_mode = editor_play_mode_e::none;
		}

		_play_snapshot.shrink(0);

		_gizmo.uninit();
		_entity_gizmo.uninit();
		_gizmo_callbacks = {};
		_custom_gizmo	 = false;
		uninstall_camera();

		for (u32 i = 0; i < EDITOR_WORLD_SNAPSHOT_SLOT_COUNT; ++i)
		{
			delete static_cast<editor_world_snapshot_data_t*>(_snapshot_slots[i].user_data);
			_snapshot_slots[i].user_data = nullptr;
		}

		_render_context.uninit();
		_edit_context.uninit();
		_world.unload_all_used_resources();
		_world.uninit();
		_snapshot_mailbox.store(0, std::memory_order_relaxed);
		_pick_result.store(0, std::memory_order_relaxed);
		_pending_pick_request		 = {};
		_last_render_pick_request_id = 0;
		_next_pick_request_id		 = 0;
		_view_rotation				 = quat_t::identity;
		_tick_callback				 = nullptr;
		_tick_callback_user_data	 = nullptr;

		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
			_object_id_readback_valid[i] = false;

		_render_resolution	  = vec2u16_t::zero;
		_producer_slot		  = 0;
		_consumer_slot		  = 0;
		_latest_snapshot_slot = UINT8_MAX;
	}

	void editor_world_t::resize(vec2u16_t render_resolution)
	{
		cancel_gizmo_action();

		_render_resolution = render_resolution;
		_render_context.resize(render_resolution);
		_world.get_screen().set_render_size(render_resolution);
		_pick_result.store(0, std::memory_order_relaxed);
		_last_render_pick_request_id = 0;

		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
			_object_id_readback_valid[i] = false;
	}

	void editor_world_t::install_camera(editor_world_camera_type_e type)
	{
		uninstall_camera();

		_camera_type = type;

		switch (type)
		{
		case editor_world_camera_type_e::orbit:
			_camera = new editor_world_camera_orbit_t();
			break;
		case editor_world_camera_type_e::fly:
		default:
			_camera = new editor_world_camera_fly_t();
			break;
		}

		_camera->init(_world);
		_edit_context.set_editor_camera_entity(_camera->get_entity());
	}

	void editor_world_t::uninstall_camera()
	{
		if (_camera == nullptr)
			return;

		cancel_gizmo_action();

		_camera->uninit(_world);
		delete _camera;
		_camera = nullptr;
		_edit_context.set_editor_camera_entity(NULL_ENTITY_ID);
	}

	void editor_world_t::serialize_camera(nlohmann::json& out_json) const
	{
		SFG_ASSERT(_camera != nullptr);
		_camera->serialize(_world, out_json);
	}

	void editor_world_t::deserialize_camera(const nlohmann::json& in_json)
	{
		SFG_ASSERT(_camera != nullptr);
		_camera->deserialize(_world, in_json);
	}

	void editor_world_t::pass_camera_input(const editor_world_camera_input_t& input)
	{
		if (_camera != nullptr)
			_camera->pass_input(_world, input);
	}

	void editor_world_t::reset_camera_input()
	{
		pass_camera_input({.reset = true});
	}

	void editor_world_t::tick_camera(f32 dt_seconds)
	{
		if (_camera != nullptr)
			_camera->tick(_world, dt_seconds);
	}

	void editor_world_t::fit_camera_to_bounds(const aabb_t& bounds)
	{
		if (_camera != nullptr)
			_camera->fit_to_bounds(_world, bounds);
	}

	void editor_world_t::set_gizmo_callbacks(const editor_gizmo_callbacks_t& callbacks)
	{
		cancel_gizmo_action();
		clear_gizmo_hover();

		_custom_gizmo	 = callbacks.get_target != nullptr;
		_gizmo_callbacks = _custom_gizmo ? callbacks : _entity_gizmo.get_callbacks();

		if (!_gizmo_callbacks.allow_scale && _edit_context.get_transform_control_type() == editor_transform_control_type_e::scale)
			_edit_context.set_transform_control_type(editor_transform_control_type_e::move);
	}

	bool editor_world_t::get_gizmo_input(editor_gizmo_input_t& input) const
	{
		if (!is_gizmo_editing_supported() || _play_mode == editor_play_mode_e::play || _play_mode == editor_play_mode_e::play_paused)
			return false;

		input.control_type = _edit_context.get_transform_control_type();
		input.locality	   = _edit_context.get_transform_locality();
		input.resolution   = _render_resolution;

		if (input.control_type == editor_transform_control_type_e::invalid || (input.control_type == editor_transform_control_type_e::scale && !_gizmo_callbacks.allow_scale))
			return false;

		if (_edit_context.get_transform_snapping() == editor_transform_snapping_e::default_)
		{
			const editor_world_view_settings_t& settings = _edit_context.get_world_view_settings();
			input.snap_translate						 = settings.snap_translate;
			input.snap_rotate							 = settings.snap_rotate;
			input.snap_scale							 = settings.snap_scale * 0.01f;
		}

		return _gizmo_callbacks.get_target(_gizmo_callbacks.user_data, input.target);
	}

	bool editor_world_t::get_gizmo_view(world_render_view_t& view)
	{
		if (_camera == nullptr)
			return false;

		const entity_id_t camera_entity = _camera->get_entity();
		vec3f_t			  scale			= vec3f_t::one;
		_world.calculate_transform_direct(camera_entity).decompose(view.pos, view.rot, scale);

		const component_camera_t& camera = _world.get_component_table(type_id_t<component_camera_t>::value).get_as_const<component_camera_t>(camera_entity);
		view.prev_pos					 = view.pos;
		view.prev_rot					 = view.rot;
		view.near_plane					 = camera.near_plane;
		view.far_plane					 = camera.far_plane;
		view.fov_degrees				 = camera.fov_degrees;
		return true;
	}

	void editor_world_t::update_gizmo_hover(vec2f_t relative_position)
	{
		world_render_view_t	 view  = {};
		editor_gizmo_input_t input = {.view = &view};

		if (get_gizmo_input(input) && get_gizmo_view(view))
			_gizmo.update_hover(input, relative_position);
		else
			_gizmo.clear_hover();
	}

	void editor_world_t::clear_gizmo_hover()
	{
		_gizmo.clear_hover();
	}

	bool editor_world_t::begin_gizmo_action(vec2f_t relative_position)
	{
		world_render_view_t	 view  = {};
		editor_gizmo_input_t input = {.view = &view};

		if (!get_gizmo_input(input) || !get_gizmo_view(view))
			return false;

		return _gizmo.begin_action(input, _gizmo_callbacks, relative_position);
	}

	void editor_world_t::update_gizmo_action(vec2f_t relative_position)
	{
		world_render_view_t	 view  = {};
		editor_gizmo_input_t input = {.view = &view};

		if (!get_gizmo_input(input) || !get_gizmo_view(view))
		{
			_gizmo.cancel_action();
			return;
		}

		_gizmo.update_action(input, relative_position);
	}

	void editor_world_t::end_gizmo_action()
	{
		editor_gizmo_input_t input = {};

		if (get_gizmo_input(input))
			_gizmo.end_action(input);
		else
			_gizmo.cancel_action();
	}

	void editor_world_t::cancel_gizmo_action()
	{
		_gizmo.cancel_action();
	}

	void editor_world_t::request_entity_pick(vec2f_t relative_position, bool incremental_selection)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD());

		if (_edit_context.get_edit_type() != editor_world_edit_type_e::full_control || _play_mode == editor_play_mode_e::play || _play_mode == editor_play_mode_e::play_paused)
			return;

		++_next_pick_request_id;

		if (_next_pick_request_id == 0)
			++_next_pick_request_id;

		_pending_pick_request = {
			.relative_position	   = relative_position,
			.id					   = _next_pick_request_id,
			.incremental_selection = incremental_selection,
		};
	}

	void editor_world_t::shoot_ray_from_camera(vec2f_t relative_position)
	{
		SFG_ASSERT(_play_mode == editor_play_mode_e::play_physics || _play_mode == editor_play_mode_e::play_physics_paused);
		SFG_ASSERT(_camera != nullptr);

		physics_world_t& physics = _world.get_physics();

		if (!physics.is_init())
			return;

		const entity_id_t camera_entity	  = _camera->get_entity();
		vec3f_t			  camera_position = _world.get_entity_pos_local(camera_entity);
		quat_t			  camera_rotation = _world.get_entity_rot_local(camera_entity);
		vec3f_t			  camera_scale	  = _world.get_entity_scale_local(camera_entity);

		const component_camera_t& camera	 = _world.get_component_table(type_id_t<component_camera_t>::value).get_as<component_camera_t>(camera_entity);
		const world_render_view_t world_view = {
			.pos		 = camera_position,
			.rot		 = camera_rotation,
			.prev_pos	 = camera_position,
			.prev_rot	 = camera_rotation,
			.near_plane	 = camera.near_plane,
			.far_plane	 = camera.far_plane,
			.fov_degrees = camera.fov_degrees,
		};
		render_view_t view = {};
		view.calculate(world_view, _render_resolution, 1.0f);

		world_ray_t ray = {};

		if (!world_util_t::relative_position_to_world_ray(view.inv_view_proj, relative_position, ray))
			return;

		ray.origin		  = camera_position;
		physics_hit_t hit = {};

		if (!physics.raycast_closest({.origin = ray.origin, .direction = ray.direction, .distance = camera.far_plane}, hit))
			return;

		const component_physical_t* body = _world.get_component_table(type_id_t<component_physical_t>::value).find_as_const<component_physical_t>(hit.entity);

		if (body == nullptr || body->motion_type != physics_motion_type_e::dynamic_body)
			return;

		physics.add_body_force(hit.entity, ray.direction * _edit_context.get_world_view_settings().physics_ray_force);
	}

	void editor_world_t::consume_entity_pick_result()
	{
		const u64 packed_result = _pick_result.exchange(0, std::memory_order_acquire);

		if (packed_result == 0)
			return;

		const u32 request_id = static_cast<u32>(packed_result >> EDITOR_WORLD_PICK_RESULT_REQUEST_SHIFT);

		if (request_id != _pending_pick_request.id)
			return;

		entity_id_t entity = static_cast<entity_id_t>(packed_result);

		if (entity != NULL_ENTITY_ID && !_world.is_alive(entity))
			entity = NULL_ENTITY_ID;

		const bool incremental_selection = _pending_pick_request.incremental_selection;
		_pending_pick_request			 = {};

		if (entity == NULL_ENTITY_ID)
		{
			_edit_context.clear_entity_selection();
			return;
		}

		if (!incremental_selection)
		{
			_edit_context.issue_entity_selection({.data = &entity, .size = 1}, entity);
			return;
		}

		const span_t<const entity_id_t> selected  = _edit_context.get_selected_entities();
		frame_vector_t<entity_id_t>		selection = {};
		selection.reserve(selected.size + 1);

		for (size_t i = 0; i < selected.size; ++i)
			selection.push_back(selected.data[i]);

		auto it = std::find(selection.begin(), selection.end(), entity);

		if (it == selection.end())
			selection.push_back(entity);
		else
			selection.erase(it);

		_edit_context.issue_entity_selection({.data = selection.data(), .size = selection.size()}, selection.empty() ? NULL_ENTITY_ID : entity);
	}

	void editor_world_t::save_play_snapshot()
	{
		SFG_ASSERT(_camera != nullptr);

		nlohmann::json snapshot = nlohmann::json::object();
		world_cooker_t::world_to_json(_world, snapshot);
		serialize_camera(snapshot["editor_camera"]);

		const string_t snapshot_text = snapshot.dump();

		_play_snapshot.shrink(0);
		_play_snapshot << snapshot_text;
	}

	void editor_world_t::restore_play_snapshot(bool keep_current_camera)
	{
		SFG_ASSERT(_play_snapshot.get_size() != 0);

		istream_t snapshot_stream(_play_snapshot.get_raw(), _play_snapshot.get_size());
		string_t  snapshot_text = {};

		snapshot_stream >> snapshot_text;

		const nlohmann::json snapshot = nlohmann::json::parse(snapshot_text, nullptr, false);
		SFG_ASSERT(!snapshot.is_discarded());
		nlohmann::json camera = snapshot.value<nlohmann::json>("editor_camera", nlohmann::json::object());

		if (keep_current_camera)
			serialize_camera(camera);

		// reset
		uninstall_camera();
		_world.reset_world_state();
		world_cooker_t::world_from_json(_world, snapshot);

		// cam
		install_camera(_camera_type);
		deserialize_camera(camera);

		// reset
		_world.update_world_transforms(false);
		_play_snapshot.shrink(0);
		_pending_pick_request		 = {};
		_last_render_pick_request_id = 0;
		_pick_result.store(0, std::memory_order_relaxed);
	}

	void editor_world_t::update_play_mode(editor_play_mode_e mode)
	{
		if (_play_mode == mode)
			return;

		if (_play_mode == editor_play_mode_e::none)
		{
			SFG_ASSERT(mode == editor_play_mode_e::play || mode == editor_play_mode_e::play_physics);

			if (mode == editor_play_mode_e::play)
			{
				cancel_gizmo_action();
				clear_gizmo_hover();
				_pending_pick_request = {};
				_pick_result.store(0, std::memory_order_relaxed);
			}

			save_play_snapshot();

			if (mode == editor_play_mode_e::play)
				uninstall_camera();

			_world.begin_play();
			_play_mode = mode;
			return;
		}

		if (mode == editor_play_mode_e::none)
		{
			const bool keep_current_camera = _play_mode == editor_play_mode_e::play_physics || _play_mode == editor_play_mode_e::play_physics_paused;
			cancel_gizmo_action();
			clear_gizmo_hover();
			_world.end_play();
			_play_mode = editor_play_mode_e::none;
			restore_play_snapshot(keep_current_camera);
			return;
		}

		const bool full_pause_transition	= (_play_mode == editor_play_mode_e::play && mode == editor_play_mode_e::play_paused) || (_play_mode == editor_play_mode_e::play_paused && mode == editor_play_mode_e::play);
		const bool physics_pause_transition = (_play_mode == editor_play_mode_e::play_physics && mode == editor_play_mode_e::play_physics_paused) || (_play_mode == editor_play_mode_e::play_physics_paused && mode == editor_play_mode_e::play_physics);
		SFG_ASSERT(full_pause_transition || physics_pause_transition);

		if (full_pause_transition && mode == editor_play_mode_e::play_paused)
			_world.pause_audio();
		else if (full_pause_transition)
			_world.resume_audio();

		_play_mode = mode;
	}

	void editor_world_t::begin_frame()
	{
		consume_entity_pick_result();
		_world.get_debug_draw().begin_frame();
	}

	void editor_world_t::invoke_tick_callback(f32 delta_time)
	{
		if (_tick_callback != nullptr)
			_tick_callback(_world, delta_time, _tick_callback_user_data);
	}

	void editor_world_t::draw_debug()
	{
		if (_edit_context.is_physics_debug_enabled() && _world.get_physics().is_init())
			_world.get_physics().draw_debug(_world.get_debug_draw());

		if (_gizmo.is_action_active())
		{
			const vec4f_t& color = editor_theme_t::get().color_accent2;

			_gizmo.draw_rotation_visualization(_world.get_debug_draw(), {color.x, color.y, color.z, color.w}, color_t::white, editor_theme_t::get().text_small_px_size * 1.5f);
		}

		const span_t<const entity_id_t> selected			 = _edit_context.get_selected_entities();
		const entity_id_t				editor_camera_entity = _edit_context.get_editor_camera_entity();

		if (_play_mode == editor_play_mode_e::none)
		{
			editor_world_util_t::draw_component_icons(_world, editor_camera_entity);

			if (selected.size != 0)
				editor_world_util_t::draw_selection_gizmos(_world, selected, _render_resolution);
		}

		if (_edit_context.is_bounding_boxes_enabled() && _latest_snapshot_slot != UINT8_MAX)
			editor_world_util_t::draw_bounding_boxes(_world, _snapshot_slots[_latest_snapshot_slot], editor_camera_entity);
	}

	void editor_world_t::update_world_transforms(bool advance_interpolation)
	{
		_world.update_world_transforms(advance_interpolation);
	}

	void editor_world_t::produce_snapshot()
	{
		world_render_snapshot_t& snapshot = _snapshot_slots[_producer_slot];
		world_snapshot_producer_t::produce(_world, snapshot, engine_runtime_t::get().get_project_settings());
		_view_rotation = snapshot.main_view.rot;

		editor_world_snapshot_data_t&	data			= *static_cast<editor_world_snapshot_data_t*>(snapshot.user_data);
		const span_t<const entity_id_t> selected		= _edit_context.get_selected_entities();
		const ecs_component_table_t&	hierarchy_table = _world.get_component_table(type_id_t<component_hierarchy_t>::value);

		data.pick_request = _pending_pick_request;
		data.grid		  = {
			.scale	 = _edit_context.get_world_view_settings().grid_scale,
			.enabled = _edit_context.is_grid_enabled(),
		};
		data.gizmo		= {};
		data.world_view = _edit_context.get_world_view();

		editor_gizmo_input_t gizmo_input = {};

		if (get_gizmo_input(gizmo_input))
		{
			const editor_gizmo_target_t& target = gizmo_input.target;
			const bool					 local	= gizmo_input.locality == editor_transform_locality_e::local;
			data.gizmo							= {
				.prev_rotation = local ? target.prev_rotation : quat_t::identity,
				.rotation	   = local ? target.rotation : quat_t::identity,
				.prev_position = target.prev_position,
				.position	   = target.position,
				.control_type  = gizmo_input.control_type,
				.hovered_axis  = _gizmo.get_hovered_axis(),
				.active_axis   = _gizmo.get_active_axis(),
			};
		}

		data.selected_entities.resize(0);
		data.selected_entities.reserve(selected.size);

		for (size_t i = 0; i < selected.size; ++i)
			data.selected_entities.push_back(selected.data[i]);

		for (size_t i = 0; i < data.selected_entities.size(); ++i)
		{
			const component_hierarchy_t& hierarchy = hierarchy_table.get_as_const<component_hierarchy_t>(data.selected_entities[i]);
			entity_id_t					 child	   = hierarchy.first_child;

			while (child != NULL_ENTITY_ID)
			{
				if (std::find(data.selected_entities.begin(), data.selected_entities.end(), child) == data.selected_entities.end())
					data.selected_entities.push_back(child);

				const component_hierarchy_t& child_hierarchy = hierarchy_table.get_as_const<component_hierarchy_t>(child);
				child										 = child_hierarchy.next_sibling;
			}
		}

		_latest_snapshot_slot = _producer_slot;
		publish_snapshot();
	}

	void editor_world_t::publish_snapshot()
	{
		const u8 prev  = _snapshot_mailbox.exchange(_producer_slot | EDITOR_WORLD_SNAPSHOT_FRESH_FLAG, std::memory_order_release);
		_producer_slot = static_cast<u8>((prev & EDITOR_WORLD_SNAPSHOT_SLOT_MASK) % EDITOR_WORLD_SNAPSHOT_SLOT_COUNT);
	}

	const world_render_snapshot_t& editor_world_t::acquire_render_snapshot()
	{
		SFG_ASSERT(SFG_IS_RENDER_THREAD() || !SFG_IS_RENDER_RUNNING());

		u8 cur = _snapshot_mailbox.load(std::memory_order_acquire);

		while (cur & EDITOR_WORLD_SNAPSHOT_FRESH_FLAG)
		{
			if (_snapshot_mailbox.compare_exchange_weak(cur, _consumer_slot, std::memory_order_acquire, std::memory_order_acquire))
			{
				_consumer_slot = static_cast<u8>((cur & EDITOR_WORLD_SNAPSHOT_SLOT_MASK) % EDITOR_WORLD_SNAPSHOT_SLOT_COUNT);
				break;
			}
		}

		return _snapshot_slots[_consumer_slot];
	}

	void editor_world_t::render(const world_render_snapshot_t& snapshot, f32 interpolation_alpha, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout)
	{
		world_render_prep_data_t prep_data = {};

		prep_data.reserve();

		const editor_world_snapshot_data_t& data = *static_cast<const editor_world_snapshot_data_t*>(snapshot.user_data);

		if (_object_id_readback_valid[frame_index] && data.pick_request.id != 0 && data.pick_request.id != _last_render_pick_request_id)
		{
			const vec2u16_t size  = _render_context.get_size();
			const vec2u16_t pixel = {
				data.pick_request.relative_position.x >= 1.0f ? static_cast<u16>(size.x - 1) : static_cast<u16>(data.pick_request.relative_position.x * size.x),
				data.pick_request.relative_position.y >= 1.0f ? static_cast<u16>(size.y - 1) : static_cast<u16>(data.pick_request.relative_position.y * size.y),
			};
			const entity_id_t entity		= _render_context.get_object_id(frame_index, pixel);
			const u64		  packed_result = static_cast<u64>(data.pick_request.id) << EDITOR_WORLD_PICK_RESULT_REQUEST_SHIFT | entity;
			_pick_result.store(packed_result, std::memory_order_release);
			_last_render_pick_request_id = data.pick_request.id;
		}

		world_rendering_t::render_world(_render_context.get_world_render_context(), snapshot, prep_data, interpolation_alpha, frame_index, global_cbv_index, global_layout);
		editor_world_rendering_t::render_outlines(_render_context, snapshot, prep_data, frame_index, global_cbv_index, global_layout);
		editor_world_rendering_t::render_object_ids(_render_context, snapshot, prep_data, frame_index);
		editor_world_rendering_t::blit_world_texture(_render_context, snapshot, prep_data, interpolation_alpha, frame_index);
		_object_id_readback_valid[frame_index] = true;
	}
}
