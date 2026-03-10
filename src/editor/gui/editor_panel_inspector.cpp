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

#include "editor_panel_inspector.hpp"
#include "editor_panel_entities.hpp"
#include "editor/editor_theme.hpp"
#include "editor/editor_settings.hpp"
#include "editor/editor.hpp"

#include "io/file_system.hpp"

#include "world/world.hpp"
#include "world/entity_manager.hpp"

#include "world/components/comp_audio.hpp"
#include "world/components/comp_camera.hpp"
#include "world/components/comp_character_controller.hpp"
#include "world/components/comp_light.hpp"
#include "world/components/comp_mesh_instance.hpp"
#include "world/components/comp_physics.hpp"

#include "platform/process.hpp"

#include "resources/entity_template.hpp"
#include "resources/entity_template_raw.hpp"
#include "resources/mesh.hpp"

#include "math/vector2ui16.hpp"
#include "math/vector3.hpp"
#include "math/quat.hpp"
#include "math/color.hpp"

#include "gui/vekt.hpp"
#include "memory/bump_text_allocator.hpp"
#include "input/input_mappings.hpp"
#include "app/app.hpp"
#include "reflection/reflection.hpp"

namespace SFG
{
	void editor_panel_inspector::init(vekt::builder* b, editor_panel_entities* entities)
	{
		_builder  = b;
		_entities = entities;

		_gui_builder.init(b);
		_root = _gui_builder.get_root();

		_gui_builder.callbacks.user_data			  = this;
		_gui_builder.callbacks.callback_ud			  = this;
		_gui_builder.callbacks.on_input_field_changed = on_input_field_changed;
		_gui_builder.callbacks.on_mouse				  = on_mouse;
		_gui_builder.callbacks.on_checkbox_changed	  = on_checkbox;
		_gui_builder.callbacks.on_control_button	  = invoke_button;

		_gui_builder.add_title("properties");
		_gui_builder.begin_area(false, false);
		_prop_vis						 = _gui_builder.add_property_row_checkbox("visible", 0).second;
		_prop_name						 = _gui_builder.add_property_row_text_field("name", "-", 256).second;
		_prop_tag						 = _gui_builder.add_property_row_text_field("tag", "-", 256).second;
		_prop_handle					 = _gui_builder.add_property_row_label("handle:", "{-, -}", 16).second;
		const gui_builder::id_quat pos	 = _gui_builder.add_property_row_vector3("position", "0.0", 16, 3, 0.1f);
		const gui_builder::id_quat rot	 = _gui_builder.add_property_row_vector3("rotation", "0.0", 16, 3, 1.0f);
		const gui_builder::id_quat scale = _gui_builder.add_property_row_vector3("scale", "1.0", 16, 3, 0.1f);
		_selected_pos_x					 = pos.second;
		_selected_pos_y					 = pos.third;
		_selected_pos_z					 = pos.fourth;

		_selected_rot_x = rot.second;
		_selected_rot_y = rot.third;
		_selected_rot_z = rot.fourth;

		_selected_scale_x = scale.second;
		_selected_scale_y = scale.third;
		_selected_scale_z = scale.fourth;

		_gui_builder.end_area();

		_gui_builder.add_title("components");
		_components_parent = _gui_builder.begin_area(true, false);

		_gui_builder.add_property_row();
		_add_component	 = _gui_builder.set_fill_x(_gui_builder.add_button("add_component").first);
		_save_template	 = _gui_builder.set_fill_x(_gui_builder.add_button("save_as_template").first);
		_unlock_template = _gui_builder.set_fill_x(_gui_builder.add_button("unlock_template").first);
		_gui_builder.pop_stack();
		_gui_builder.end_area();

		_comp_header_buttons.reserve(512);
		_add_component_buttons.reserve(512);
		_selection_debug_draws.reserve(8);

		set_selected_controls();
	}

	void editor_panel_inspector::uninit()
	{
		_gui_builder.uninit();
	}

	void editor_panel_inspector::draw(world& w, const vector2ui16& window_size)
	{
		sync_selected(w);

		entity_manager&	   em = w.get_entity_manager();
		component_manager& cm = w.get_comp_manager();
		if (w.get_playmode() == play_mode::none)
		{
			const f32 thickness	  = 0.05f;
			const color col_phy		  = color::red;
			const color col_light	  = color::yellow;
			const color col_light_alt = color::red;

			world_debug_rendering& debug_rendering = w.get_debug_rendering();

			const vector3 selected_pos	 = _selected_entity.is_null() ? vector3::zero : em.get_entity_position_abs(_selected_entity);
			const quat	  selected_rot	 = _selected_entity.is_null() ? quat::identity : em.get_entity_rotation_abs(_selected_entity);
			const vector3 selected_scale = _selected_entity.is_null() ? vector3::one : em.get_entity_scale_abs(_selected_entity);

			for (const selection_debug_draw& dd : _selection_debug_draws)
			{
				if (dd.type == debug_draw_type::physics)
				{
					const comp_physics&		 c	 = cm.get_component<comp_physics>(dd.component);
					const physics_shape_type st	 = c.get_shape_type();
					const vector3&			 val = c.get_extent_or_rad_height();

					const vector3 orientation = selected_rot.get_up();
					if (st == physics_shape_type::box)
						debug_rendering.draw_box(selected_pos + c.get_offset(), val, selected_rot.get_forward(), color::red, thickness);
					else if (st == physics_shape_type::capsule)
						debug_rendering.draw_capsule(selected_pos + c.get_offset(), val.x * vector2(selected_scale.x, selected_scale.z).magnitude(), val.y * 0.5f * selected_scale.y, orientation, col_phy, thickness);
					else if (st == physics_shape_type::cylinder)
						debug_rendering.draw_cylinder(selected_pos + c.get_offset(), val.x * vector2(selected_scale.x, selected_scale.z).magnitude(), val.y * selected_scale.y, orientation, col_phy, thickness);
					else if (st == physics_shape_type::mesh)
					{
						const world_handle mesh_comp = em.get_entity_component<comp_mesh_instance>(c.get_header().entity);
						if (!mesh_comp.is_null())
						{
							const comp_mesh_instance& mi = cm.get_component<comp_mesh_instance>(mesh_comp);
							const resource_handle	  mh = mi.get_mesh();
							resource_manager&		  rm = w.get_resource_manager();
							if (rm.is_valid<mesh>(mh))
							{
								const mesh&	 res	   = rm.get_resource<mesh>(mh);
								const u32 vtx_count = res.get_collider_vertex_count();
								const u32 idx_count = res.get_collider_index_count();
								if (vtx_count != 0 && idx_count != 0)
								{
									const vector3*		   vtx	= rm.get_aux().get<vector3>(res.get_collider_vertices());
									const primitive_index* idx	= rm.get_aux().get<primitive_index>(res.get_collider_indices());
									const vector3		   base = selected_pos + c.get_offset();
									for (u32 i = 0; i + 2 < idx_count; i += 3)
									{
										const vector3 p0 = selected_rot * (vtx[idx[i + 0]] * selected_scale) + base;
										const vector3 p1 = selected_rot * (vtx[idx[i + 1]] * selected_scale) + base;
										const vector3 p2 = selected_rot * (vtx[idx[i + 2]] * selected_scale) + base;
										debug_rendering.draw_triangle(p0, p1, p2, col_phy);
									}
								}
							}
						}
					}
					else if (st == physics_shape_type::sphere)
						debug_rendering.draw_sphere(selected_pos + c.get_offset(), val.x * selected_scale.magnitude(), col_phy, thickness);
					else if (st == physics_shape_type::plane)
						debug_rendering.draw_oriented_plane(selected_pos + c.get_offset(), val.x * selected_scale.x, val.z * selected_scale.z, vector3::up, col_phy, thickness);
				}
				else if (dd.type == debug_draw_type::character_controller)
				{
					const comp_character_controller& c			 = cm.get_component<comp_character_controller>(dd.component);
					const f32						 radius		 = c.get_radius() * vector2(selected_scale.x, selected_scale.z).magnitude();
					const f32						 half_height = c.get_half_height() * selected_scale.y;
					debug_rendering.draw_capsule(selected_pos + c.get_shape_offset(), radius, half_height, selected_rot.get_up(), col_phy, thickness);
				}
				else if (dd.type == debug_draw_type::audio)
				{
					const comp_audio& c = cm.get_component<comp_audio>(dd.component);
					if (c.get_attenuation() != sound_attenuation::none)
					{
						debug_rendering.draw_sphere(selected_pos, c.get_radius_min(), col_light_alt, thickness);
						debug_rendering.draw_sphere(selected_pos, c.get_radius_max(), col_light, thickness);
					}
				}
				else if (dd.type == debug_draw_type::camera)
				{
					const comp_camera& c = cm.get_component<comp_camera>(dd.component);
					debug_rendering.draw_frustum(selected_pos, selected_rot.get_forward(), c.get_fov_degrees(), 1.0f, c.get_near(), c.get_far(), col_light, thickness);
				}
				else if (dd.type == debug_draw_type::point_light)
				{
					const comp_point_light& c = cm.get_component<comp_point_light>(dd.component);
					debug_rendering.draw_sphere(selected_pos, c.get_range(), col_light, thickness);
				}
				else if (dd.type == debug_draw_type::spot_light)
				{
					const comp_spot_light& c = cm.get_component<comp_spot_light>(dd.component);
					debug_rendering.draw_oriented_cone(selected_pos, selected_rot.get_forward(), c.get_range(), c.get_inner_cone(), col_light_alt, thickness, 12);
					debug_rendering.draw_oriented_cone(selected_pos, selected_rot.get_forward(), c.get_range(), c.get_outer_cone(), col_light, thickness, 12);
				}
			}
		}

		const world_handle	 selected = _selected_entity;
		bump_text_allocator& alloc	  = editor::get().get_bump_text_allocator();

		const char* name_txt = selected.is_null() ? "-" : em.get_entity_meta(selected).name;
		const char* tag_txt	 = selected.is_null() ? "-" : em.get_entity_meta(selected).tag;
		_gui_builder.set_text_field_text(_prop_name, name_txt, true);
		_gui_builder.set_text_field_text(_prop_tag, tag_txt, true);
		_gui_builder.set_checkbox_value(_prop_vis, selected.is_null() ? 0 : !em.get_entity_flags(selected).is_set(entity_flags::entity_flags_invisible));
		{
			const char* handle_txt = alloc.allocate_reserve(32);
			if (selected.is_null())
			{
				alloc.append("-");
			}
			else
			{
				alloc.append("[");
				alloc.append(static_cast<u32>(selected.generation));
				alloc.append(", ");
				alloc.append(static_cast<u32>(selected.index));
				alloc.append("]");
			}

			_builder->widget_get_text(_prop_handle).text = handle_txt;
			_builder->widget_update_text(_prop_handle);
		}

		{
			if (selected.is_null())
			{
				_gui_builder.set_text_field_value(_selected_pos_x, 0.0f, true);
				_gui_builder.set_text_field_value(_selected_pos_y, 0.0f, true);
				_gui_builder.set_text_field_value(_selected_pos_z, 0.0f, true);
				_gui_builder.set_text_field_value(_selected_rot_x, 0.0f, true);
				_gui_builder.set_text_field_value(_selected_rot_y, 0.0f, true);
				_gui_builder.set_text_field_value(_selected_rot_z, 0.0f, true);
				_gui_builder.set_text_field_value(_selected_scale_x, 0.0f, true);
				_gui_builder.set_text_field_value(_selected_scale_y, 0.0f, true);
				_gui_builder.set_text_field_value(_selected_scale_z, 0.0f, true);
			}
			else
			{
				const vector3& pos	 = em.get_entity_position(selected);
				const quat&	   rot	 = em.get_entity_rotation(selected);
				const vector3& scale = em.get_entity_scale(selected);
				const vector3& eul	 = quat::to_euler(rot);
				_gui_builder.set_text_field_value(_selected_pos_x, pos.x, true);
				_gui_builder.set_text_field_value(_selected_pos_y, pos.y, true);
				_gui_builder.set_text_field_value(_selected_pos_z, pos.z, true);
				_gui_builder.set_text_field_value(_selected_rot_x, eul.x, true);
				_gui_builder.set_text_field_value(_selected_rot_y, eul.y, true);
				_gui_builder.set_text_field_value(_selected_rot_z, eul.z, true);
				_gui_builder.set_text_field_value(_selected_scale_x, scale.x, true);
				_gui_builder.set_text_field_value(_selected_scale_y, scale.y, true);
				_gui_builder.set_text_field_value(_selected_scale_z, scale.z, true);
			}
		}
	}

	void editor_panel_inspector::on_context_item(vekt::id widget)
	{
		_gui_builder.on_context_item(widget);

		if (_selected_entity.is_null() || _add_component_buttons.empty())
			return;

		for (const add_comp_button& b : _add_component_buttons)
		{
			if (b.button == widget)
			{
				world&						w		 = editor::get().get_app().get_world();
				entity_manager&				em		 = w.get_entity_manager();
				component_manager&			cm		 = w.get_comp_manager();
				const entity_comp_register& reg		 = em.get_component_register(_selected_entity);
				bool						has_comp = false;
				for (const entity_comp& c : reg.comps)
				{
					if (c.comp_type == b.type)
					{
						has_comp = true;
						break;
					}
				}

				if (has_comp)
				{
					open_info_popup("entity already has this component");
					_add_component_buttons.resize(0);
					return;
				}

				const world_handle new_handle = cm.add_component(b.type, _selected_entity);
				if (new_handle.is_null())
				{
					open_info_popup("failed to add component, this component's limit has been reached.");
					_add_component_buttons.resize(0);
					return;
				}

				clear_component_view();
				build_component_view();
				_add_component_buttons.resize(0);
				em.set_hierarchy_dirty(1);
				return;
			}
		}
	}

	void editor_panel_inspector::on_popup_item(vekt::id widget)
	{
	}

	void editor_panel_inspector::clear_component_view()
	{
		_selection_debug_draws.resize(0);

		if (_components_area != NULL_WIDGET_ID)
			_gui_builder.deallocate(_components_area);
		_components_area = NULL_WIDGET_ID;

		_comp_header_buttons.resize(0);
	}

	bool editor_panel_inspector::is_component_collapsed(string_id type_id) const
	{
		for (const component_collapse_state& state : _component_collapse_states)
		{
			if (state.comp_type == type_id)
				return state.collapsed;
		}

		return false;
	}

	void editor_panel_inspector::set_component_collapsed(string_id type_id, bool collapsed)
	{
		for (component_collapse_state& state : _component_collapse_states)
		{
			if (state.comp_type == type_id)
			{
				state.collapsed = collapsed;
				return;
			}
		}

		_component_collapse_states.push_back({.comp_type = type_id, .collapsed = collapsed});
	}

	void editor_panel_inspector::build_component_view()
	{
		world&			w  = editor::get().get_app().get_world();
		entity_manager& em = w.get_entity_manager();
		if (em.get_entity_flags(_selected_entity).is_set(entity_flags::entity_flags_template))
			return;

		_gui_builder.push_stack(_components_parent);
		_components_area = _gui_builder.begin_area(true, true);

		_gui_builder.set_draw_order(1);

		component_manager&			cm = w.get_comp_manager();
		const entity_comp_register& cr = em.get_component_register(_selected_entity);
		for (const entity_comp& c : cr.comps)
		{
			const meta&				   m			= reflection::get().resolve(c.comp_type);
			const meta::field_vec&	   fields		= m.get_fields();
			void*					   comp_ptr		= cm.get_component(c.comp_type, c.comp_handle);
			const bool				   is_collapsed = is_component_collapsed(c.comp_type);
			const gui_builder::id_trip pair			= _gui_builder.add_component_title(m.get_title().c_str());
			_gui_builder.set_indent(editor_theme::get().outer_margin * 3);

			_comp_header_buttons.push_back({c.comp_handle, pair.first, pair.second, pair.third, c.comp_type});

			if (!is_collapsed)
			{
				for (field_base* f : fields)
				{
					_gui_builder.add_reflected_field(f, c.comp_type, comp_ptr);
				}

				const meta::button_vec& controls = m.get_control_buttons();

				if (!controls.empty())
				{
					const vekt::id id								  = _gui_builder.add_property_row();
					_builder->widget_get_size_props(id).child_margins = {0.0f, 0.0f, editor_theme::get().inner_margin, editor_theme::get().outer_margin};
				}

				for (const control_button& button : controls)
				{
					_gui_builder.set_fill_x(_gui_builder.add_control_button(button.title.c_str(), c.comp_type, comp_ptr, button.sid));
				}

				if (!controls.empty())
					_gui_builder.pop_stack();
			}

			if (c.comp_type == type_id<comp_physics>::value)
			{
				_selection_debug_draws.push_back({
					.type	   = debug_draw_type::physics,
					.component = c.comp_handle,
				});
			}
			else if (c.comp_type == type_id<comp_character_controller>::value)
			{
				_selection_debug_draws.push_back({
					.type	   = debug_draw_type::character_controller,
					.component = c.comp_handle,
				});
			}
			else if (c.comp_type == type_id<comp_audio>::value)
			{
				_selection_debug_draws.push_back({
					.type	   = debug_draw_type::audio,
					.component = c.comp_handle,
				});
			}
			else if (c.comp_type == type_id<comp_camera>::value)
			{
				_selection_debug_draws.push_back({
					.type	   = debug_draw_type::camera,
					.component = c.comp_handle,
				});
			}
			else if (c.comp_type == type_id<comp_point_light>::value)
			{
				_selection_debug_draws.push_back({
					.type	   = debug_draw_type::point_light,
					.component = c.comp_handle,
				});
			}
			else if (c.comp_type == type_id<comp_spot_light>::value)
			{
				_selection_debug_draws.push_back({
					.type	   = debug_draw_type::spot_light,
					.component = c.comp_handle,
				});
			}
		}
		_gui_builder.set_indent(0);
		_gui_builder.set_draw_order(0);

		_gui_builder.end_area();
		_gui_builder.pop_stack();

		_builder->build_hierarchy();
	}

	void editor_panel_inspector::set_selected_controls()
	{
		bool is_template = 0;
		if (!_selected_entity.is_null())
			is_template = editor::get().get_app().get_world().get_entity_manager().get_entity_flags(_selected_entity).is_set(entity_flags::entity_flags_template);
		const bool template_disabled = _selected_entity.is_null() || is_template;

		_gui_builder.set_widget_enabled(_unlock_template, is_template, editor_theme::get().col_button, editor_theme::get().col_button_silent);
		_gui_builder.set_widget_enabled(_add_component, !template_disabled, editor_theme::get().col_button, editor_theme::get().col_button_silent);
		_gui_builder.set_widget_enabled(_save_template, !template_disabled, editor_theme::get().col_button, editor_theme::get().col_button_silent);
		_gui_builder.set_widget_enabled(_prop_name, !template_disabled, editor_theme::get().col_frame_bg, editor_theme::get().col_button_silent);
		_gui_builder.set_widget_enabled(_prop_tag, !template_disabled, editor_theme::get().col_frame_bg, editor_theme::get().col_button_silent);
		_gui_builder.set_widget_enabled(_selected_pos_x, !_selected_entity.is_null(), editor_theme::get().col_frame_bg, editor_theme::get().col_button_silent);
		_gui_builder.set_widget_enabled(_selected_pos_y, !_selected_entity.is_null(), editor_theme::get().col_frame_bg, editor_theme::get().col_button_silent);
		_gui_builder.set_widget_enabled(_selected_pos_z, !_selected_entity.is_null(), editor_theme::get().col_frame_bg, editor_theme::get().col_button_silent);
		_gui_builder.set_widget_enabled(_selected_rot_x, !_selected_entity.is_null(), editor_theme::get().col_frame_bg, editor_theme::get().col_button_silent);
		_gui_builder.set_widget_enabled(_selected_rot_y, !_selected_entity.is_null(), editor_theme::get().col_frame_bg, editor_theme::get().col_button_silent);
		_gui_builder.set_widget_enabled(_selected_rot_z, !_selected_entity.is_null(), editor_theme::get().col_frame_bg, editor_theme::get().col_button_silent);
		_gui_builder.set_widget_enabled(_selected_scale_x, !_selected_entity.is_null(), editor_theme::get().col_frame_bg, editor_theme::get().col_button_silent);
		_gui_builder.set_widget_enabled(_selected_scale_y, !_selected_entity.is_null(), editor_theme::get().col_frame_bg, editor_theme::get().col_button_silent);
		_gui_builder.set_widget_enabled(_selected_scale_z, !_selected_entity.is_null(), editor_theme::get().col_frame_bg, editor_theme::get().col_button_silent);
		_gui_builder.set_widget_enabled(_prop_vis, !_selected_entity.is_null(), editor_theme::get().col_frame_bg, editor_theme::get().col_button_silent);
		_gui_builder.set_widget_enabled(_prop_handle, !_selected_entity.is_null(), editor_theme::get().col_text, editor_theme::get().col_button_silent);
		_builder->build_hierarchy();
	}

	void editor_panel_inspector::sync_selected(world& w)
	{
		entity_manager& em		= w.get_entity_manager();
		world_handle	current = _entities ? _entities->get_selected() : world_handle{};
		if (!current.is_null() && !em.is_valid(current))
			current = {};

		if (_selected_entity == current)
			return;

		_selected_entity = current;
		set_selected_controls();

		clear_component_view();

		if (_selected_entity.is_null())
			return;

		build_component_view();
	}

	void editor_panel_inspector::open_info_popup(const char* text)
	{
		editor_gui_controller& ctr = editor::get().get_gui_controller();
		ctr.begin_popup(text);
		ctr.popup_add_button("ok");
		ctr.end_popup();
	}

	void editor_panel_inspector::on_input_field_changed(void* callback_ud, vekt::builder* b, vekt::id widget, const char* txt, f32 value)
	{
		editor_panel_inspector* self = static_cast<editor_panel_inspector*>(callback_ud);

		world&			w  = editor::get().get_app().get_world();
		entity_manager& em = w.get_entity_manager();

		if (!em.is_valid(self->_selected_entity))
			return;

		const vector3& pos	 = em.get_entity_position(self->_selected_entity);
		const quat&	   rot	 = em.get_entity_rotation(self->_selected_entity);
		const vector3& scale = em.get_entity_scale(self->_selected_entity);
		const vector3& eul	 = quat::to_euler(rot);

		if (widget == self->_prop_name)
		{
			em.set_entity_name(self->_selected_entity, txt);
			if (self->_entities)
				self->_entities->update_entity_name(self->_selected_entity);
		}
		else if (widget == self->_prop_tag)
		{
			em.set_entity_tag(self->_selected_entity, txt);
		}
		else if (widget == self->_prop_vis)
		{
		}
		else if (widget == self->_selected_pos_x)
		{
			em.set_entity_position(self->_selected_entity, vector3(value, pos.y, pos.z));
		}
		else if (widget == self->_selected_pos_y)
		{
			em.set_entity_position(self->_selected_entity, vector3(pos.x, value, pos.z));
		}
		else if (widget == self->_selected_pos_z)
		{
			em.set_entity_position(self->_selected_entity, vector3(pos.x, pos.y, value));
		}
		else if (widget == self->_selected_rot_x)
		{
			em.set_entity_rotation(self->_selected_entity, quat::from_euler(value, eul.y, eul.z));
		}
		else if (widget == self->_selected_rot_y)
		{
			em.set_entity_rotation(self->_selected_entity, quat::from_euler(eul.x, value, eul.z));
		}
		else if (widget == self->_selected_rot_z)
		{
			em.set_entity_rotation(self->_selected_entity, quat::from_euler(eul.x, eul.y, value));
		}
		else if (widget == self->_selected_scale_x)
		{
			em.set_entity_scale(self->_selected_entity, vector3(value, scale.y, scale.z));
		}
		else if (widget == self->_selected_scale_y)
		{
			em.set_entity_scale(self->_selected_entity, vector3(scale.x, value, scale.z));
		}
		else if (widget == self->_selected_scale_z)
		{
			em.set_entity_scale(self->_selected_entity, vector3(scale.x, scale.y, value));
		}
	}

	void editor_panel_inspector::on_checkbox(void* callback_ud, vekt::builder* b, vekt::id id, unsigned char value)
	{
		editor_panel_inspector* self = static_cast<editor_panel_inspector*>(callback_ud);
		if (!self->_selected_entity.is_null() && id == self->_prop_vis)
		{
			world&			w  = editor::get().get_app().get_world();
			entity_manager& em = w.get_entity_manager();
			em.set_entity_visible(self->_selected_entity, value);
		}
	}

	void editor_panel_inspector::invoke_button(void* callback_ud, void* object_ptr, string_id type_id, string_id button_id)
	{
		if (object_ptr == nullptr || type_id == 0 || button_id == 0)
			return;

		meta& m = reflection::get().resolve(type_id);
		if (!m.has_function("on_button"_hs))
			return;

		const reflected_button_params params = {
			.w			= editor::get().get_app().get_world(),
			.object_ptr = object_ptr,
			.button_id	= button_id,
		};
		m.invoke_function<void, const reflected_button_params&>("on_button"_hs, params);
	}

	vekt::input_event_result editor_panel_inspector::on_mouse(vekt::builder* b, vekt::id widget, const vekt::mouse_event& ev, vekt::input_event_phase phase)
	{
		editor_panel_inspector* self = static_cast<editor_panel_inspector*>(b->widget_get_user_data(widget).ptr);

		if (ev.type == vekt::input_event_type::released && ev.button == static_cast<u16>(input_code::mouse_0))
		{
			if (self->_gui_builder.invoke_control_button(widget))
				return vekt::input_event_result::handled;
		}

		if (ev.type == vekt::input_event_type::released && widget == self->_add_component && !self->_selected_entity.is_null())
		{
			self->_add_component_buttons.resize(0);
			editor::get().get_gui_controller().begin_context_menu(ev.position.x, ev.position.y);

			const auto& metas = reflection::get().get_metas();

			struct meta_category
			{
				string			  title = "";
				vector<string_id> tids;
			};

			vector<meta_category> categories;

			for (const auto& entry : metas)
			{
				const meta& meta = entry.meta;
				if (meta.get_tag() == "component"_hs && !meta.get_title().empty())
				{
					const string title = meta.get_category().c_str();
					auto		 it	   = std::find_if(categories.begin(), categories.end(), [&](const meta_category& c) -> bool { return c.title.compare(title) == 0; });

					if (it == categories.end())
						categories.push_back({.title = title, .tids = {meta.get_type_id()}});
					else
						it->tids.push_back(meta.get_type_id());
				}
			}

			for (const meta_category& cat : categories)
			{
				editor::get().get_gui_controller().add_context_menu_title(cat.title.c_str());

				for (string_id tid : cat.tids)
				{
					const meta&	   meta	  = reflection::get().resolve(tid);
					const vekt::id button = editor::get().get_gui_controller().add_context_menu_item(meta.get_title().c_str());
					self->_add_component_buttons.push_back({.button = button, .type = meta.get_type_id()});
				}
			}

			editor::get().get_gui_controller().end_context_menu();
			return vekt::input_event_result::handled;
		}

		if (ev.type == vekt::input_event_type::released && widget == self->_save_template && !self->_selected_entity.is_null())
		{
			world&		 w	  = editor::get().get_app().get_world();
			const string file = process::save_file("save entity file", ".stkent");
			if (file.empty())
				return vekt::input_event_result::handled;

			entity_template_raw::save_to_file(file.c_str(), w, {self->_selected_entity});

			string relative = editor_settings::get().get_relative(file);
			file_system::fix_path(relative);
			w.get_resource_manager().load_resources({relative});

			const resource_handle h = w.get_resource_manager().get_resource_handle_by_hash<entity_template>(TO_SID(relative));
			w.get_entity_manager().set_entity_template(self->_selected_entity, h);
			if (self->_entities)
				self->_entities->set_entity_collapsed(self->_selected_entity, true);

			self->set_selected_controls();
			self->clear_component_view();
			self->build_component_view();
			return vekt::input_event_result::handled;
		}

		if (ev.type == vekt::input_event_type::released && widget == self->_unlock_template && !self->_selected_entity.is_null())
		{
			world& w = editor::get().get_app().get_world();
			w.get_entity_manager().set_entity_template(self->_selected_entity, {});
			if (self->_entities)
				self->_entities->set_entity_collapsed(self->_selected_entity, false);
			self->set_selected_controls();
			self->clear_component_view();
			self->build_component_view();
			return vekt::input_event_result::handled;
		}

		if (ev.type == vekt::input_event_type::released && !self->_selected_entity.is_null())
		{
			for (const comp_header_button& b : self->_comp_header_buttons)
			{
				if (widget == b.collapse_row)
				{
					const bool collapsed = self->is_component_collapsed(b.comp_type);
					self->set_component_collapsed(b.comp_type, !collapsed);
					self->clear_component_view();
					self->build_component_view();
					return vekt::input_event_result::handled;
				}
			}
		}

		if (!self->_selected_entity.is_null())
		{
			for (const comp_header_button& b : self->_comp_header_buttons)
			{
				if (widget == b.reset_button)
				{
					// block but do nothing
					if (ev.type == vekt::input_event_type::pressed)
						return vekt::input_event_result::handled;

					world&			   w  = editor::get().get_app().get_world();
					entity_manager&	   em = w.get_entity_manager();
					component_manager& cm = w.get_comp_manager();
					cm.remove_component(b.comp_type, self->_selected_entity, b.handle);
					const world_handle new_handle = cm.add_component(b.comp_type, self->_selected_entity);
					if (new_handle.is_null())
					{
						self->open_info_popup("failed to add component, this component's limit has been reached.");
						self->clear_component_view();
						self->build_component_view();
						em.set_hierarchy_dirty(1);
						return vekt::input_event_result::handled;
					}

					meta& m = reflection::get().resolve(b.comp_type);
					if (m.has_function("on_reflect_load"_hs))
					{
						void* comp_ptr = cm.get_component(b.comp_type, new_handle);
						m.invoke_function<void, void*, world&>("on_reflect_load"_hs, comp_ptr, w);
					}
					self->clear_component_view();
					self->build_component_view();
					em.set_hierarchy_dirty(1);
					return vekt::input_event_result::handled;
				}
			}
		}

		if (ev.type == vekt::input_event_type::released && !self->_selected_entity.is_null())
		{
			for (const comp_header_button& b : self->_comp_header_buttons)
			{
				if (widget == b.remove_button)
				{
					world&			   w  = editor::get().get_app().get_world();
					entity_manager&	   em = w.get_entity_manager();
					component_manager& cm = w.get_comp_manager();
					cm.remove_component(b.comp_type, self->_selected_entity, b.handle);
					self->clear_component_view();
					self->build_component_view();
					em.set_hierarchy_dirty(1);
					return vekt::input_event_result::handled;
				}
			}
		}
		return vekt::input_event_result::not_handled;
	}
}
