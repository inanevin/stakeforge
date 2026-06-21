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

#include "input_router.hpp"
#include <sfg/input/input_mappings.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/math/math.hpp>

#include <algorithm>

namespace sfg::ui
{
	namespace
	{
		bool point_in_rect(const vec4f_t& r, const vec2f_t& p)
		{
			return p.x >= r.x && p.x <= r.x + r.z && p.y >= r.y && p.y <= r.y + r.w;
		}
	}

	void input_router_t::init(const input_config_t& cfg, u32 max_widgets)
	{
		_config = cfg;
		_listeners.clear();
		_focus_order.init(max_widgets);
		_hit_order.init(max_widgets);
		_hovered = NULL_WIDGET;
		_focused = NULL_WIDGET;
		for (u32 i = 0; i < static_cast<u32>(mouse_button_e::count); ++i)
		{
			_pressed[i]		  = NULL_WIDGET;
			_pressed_state[i] = {};
			_last_click[i]	  = {};
		}
	}

	void input_router_t::uninit()
	{
		_listeners.clear();
		_hit_order.uninit();
		_focus_order.uninit();
	}

	void input_router_t::set_listener(widget_id_t id, const listener_bundle_t& b)
	{
		_listeners[id] = b;
	}

	void input_router_t::clear_listener(widget_id_t id)
	{
		_listeners.erase(id);
	}

	void input_router_t::clear_widget_state(widget_id_t id)
	{
		clear_listener(id);

		if (_hovered == id)
			_hovered = NULL_WIDGET;
		if (_focused == id)
			_focused = NULL_WIDGET;

		for (u32 i = 0; i < static_cast<u32>(mouse_button_e::count); ++i)
		{
			if (_pressed[i] == id)
			{
				_pressed[i]		  = NULL_WIDGET;
				_pressed_state[i] = {};
			}
			if (_last_click[i].target == id)
				_last_click[i] = {};
		}

		if (_popup_scope.owner_root == id)
		{
			clear_popup_scope();
			return;
		}

		for (u32 i = 0; i < _popup_scope.popup_root_count; ++i)
		{
			if (_popup_scope.popup_roots[i] == id)
			{
				clear_popup_scope();
				return;
			}
		}
	}

	void input_router_t::set_popup_scope(widget_id_t owner_root, const widget_id_t* popup_roots, u32 popup_root_count, on_popup_outside_press_fn on_outside_press, void* user_data, popup_hover_policy_e hover_policy)
	{
		SFG_ASSERT(popup_root_count <= POPUP_SCOPE_MAX_ROOTS);

		_popup_scope.owner_root		  = owner_root;
		_popup_scope.previous_focus	  = _focused;
		_popup_scope.popup_root_count = popup_root_count;
		_popup_scope.on_outside_press = on_outside_press;
		_popup_scope.user_data		  = user_data;
		_popup_scope.hover_policy	  = hover_policy;
		_popup_scope.active			  = true;

		for (u32 i = 0; i < popup_root_count; ++i)
			_popup_scope.popup_roots[i] = popup_roots[i];
		for (u32 i = popup_root_count; i < POPUP_SCOPE_MAX_ROOTS; ++i)
			_popup_scope.popup_roots[i] = NULL_WIDGET;
	}

	void input_router_t::clear_popup_scope()
	{
		const popup_scope_t scope = _popup_scope;
		if (scope.active && _focused != NULL_WIDGET && is_in_popup_scope(_focused))
		{
			_popup_scope = {};
			if (scope.previous_focus != NULL_WIDGET && _tree != nullptr && _tree->is_alive(scope.previous_focus))
			{
				const layout_in_t& in = _tree->in_const(scope.previous_focus);
				if ((in.flags & wf_visible) != 0 && (in.flags & wf_input) != 0 && (in.flags & wf_focusable) != 0 && (in.flags & wf_disabled) == 0)
				{
					set_focus(scope.previous_focus, false);
					return;
				}
			}
			set_focus(NULL_WIDGET, false);
			return;
		}
		_popup_scope = {};
	}

	void input_router_t::rebuild_hit_test(const layout_tree_t& tree)
	{
		_hit_order.resize(0);
		_focus_order.resize(0);

		const auto dfs = tree.get_dfs();

		for (size_t i = 0; i < dfs.size; ++i)
		{
			const widget_id_t id = dfs.data[i];
			if (id == tree.get_root())
				continue;

			const layout_in_t& in = tree.in_const(id);
			if (in.flags & wf_visible)
			{
				const layout_out_t& out		 = tree.out(id);
				const bool			disabled = (in.flags & wf_disabled) != 0;
				if (in.flags & wf_input)
					_hit_order.push_back(id);
				if ((in.flags & wf_input) && (in.flags & wf_focusable) && !disabled && out.clip.z > 0.0f && out.clip.w > 0.0f)
					_focus_order.push_back(id);
			}
		}

		std::stable_sort(_hit_order.begin(), _hit_order.end(), [&](widget_id_t a, widget_id_t b) {
			const u32 da = tree.draw_order_const(a);
			const u32 db = tree.draw_order_const(b);
			return da > db;
		});
	}

	widget_id_t input_router_t::raw_hit_test(const vec2f_t& pos) const
	{
		for (widget_id_t id : _hit_order)
		{
			const layout_out_t& out = _tree->out(id);
			if (out.clip.z <= 0.0f || out.clip.w <= 0.0f)
				continue;
			if (point_in_rect(out.clip, pos))
			{
				if (_tree->in_const(id).flags & wf_disabled)
					return NULL_WIDGET;
				return id;
			}
		}
		return NULL_WIDGET;
	}

	bool input_router_t::is_in_subtree(widget_id_t id, widget_id_t root) const
	{
		widget_id_t cur = id;
		while (cur != NULL_WIDGET)
		{
			if (cur == root)
				return true;
			cur = _tree->node(cur).parent;
		}
		return false;
	}

	widget_id_t input_router_t::find_focus_target(widget_id_t id) const
	{
		if (_tree == nullptr)
			return NULL_WIDGET;

		widget_id_t cur = id;
		while (cur != NULL_WIDGET)
		{
			const layout_in_t& in = _tree->in_const(cur);
			if ((in.flags & wf_input) != 0 && (in.flags & wf_focusable) != 0 && (in.flags & wf_disabled) == 0)
				return cur;
			cur = _tree->node(cur).parent;
		}
		return NULL_WIDGET;
	}

	bool input_router_t::is_in_popup_scope(widget_id_t id) const
	{
		if (!_popup_scope.active || id == NULL_WIDGET)
			return false;
		if (is_in_subtree(id, _popup_scope.owner_root))
			return true;
		for (u32 i = 0; i < _popup_scope.popup_root_count; ++i)
		{
			if (is_in_subtree(id, _popup_scope.popup_roots[i]))
				return true;
		}
		return false;
	}

	widget_id_t input_router_t::hit_test(const vec2f_t& pos) const
	{
		const widget_id_t target = raw_hit_test(pos);
		if (!_popup_scope.active)
			return target;
		if (is_in_popup_scope(target))
			return target;
		if (_popup_scope.hover_policy == popup_hover_policy_e::pass_outside)
			return target;
		return NULL_WIDGET;
	}

	void input_router_t::fire_hover_change(widget_id_t new_hover)
	{
		if (new_hover == _hovered)
		{
			if (_hovered != NULL_WIDGET)
			{
				auto it = _listeners.find(_hovered);
				if (it != _listeners.end() && it->second.on_hover_move)
					it->second.on_hover_move(*this, _hovered, _mouse, _mouse - _mouse_prev, it->second.user_data);
			}
			return;
		}

		if (_hovered != NULL_WIDGET)
		{
			auto it = _listeners.find(_hovered);
			if (it != _listeners.end() && it->second.on_hover_exit)
				it->second.on_hover_exit(*this, _hovered, _mouse, {0, 0}, it->second.user_data);
		}

		_hovered = new_hover;

		if (_hovered != NULL_WIDGET)
		{
			auto it = _listeners.find(_hovered);
			if (it != _listeners.end() && it->second.on_hover_enter)
				it->second.on_hover_enter(*this, _hovered, _mouse, {0, 0}, it->second.user_data);
		}
	}

	void input_router_t::tick(const layout_tree_t& tree, f32 dt_seconds)
	{
		_tree = &tree;
		_accum_time += dt_seconds;

		rebuild_hit_test(tree);

		const widget_id_t target = hit_test(_mouse);
		if (target != _hovered)
			fire_hover_change(target);

		for (u32 i = 0; i < static_cast<u32>(mouse_button_e::count); ++i)
		{
			press_state_t& ps = _pressed_state[i];
			if (_pressed[i] == NULL_WIDGET)
				continue;
			ps.held_seconds += dt_seconds;

			const vec2f_t delta = _mouse - ps.press_pos;
			if (!ps.dragging && (math::abs(delta.x) > _config.drag_threshold_pixels || math::abs(delta.y) > _config.drag_threshold_pixels))
			{
				ps.dragging = true;
				auto it		= _listeners.find(_pressed[i]);
				if (it != _listeners.end() && it->second.on_drag_begin)
					it->second.on_drag_begin(*this, _pressed[i], _mouse, delta, it->second.user_data);
			}

			if (ps.dragging)
			{
				auto it = _listeners.find(_pressed[i]);
				if (it != _listeners.end() && it->second.on_drag)
					it->second.on_drag(*this, _pressed[i], _mouse, _mouse - _mouse_prev, it->second.user_data);
			}
		}

		_mouse_prev = _mouse;
	}

	void input_router_t::on_mouse_move(const vec2f_t& pos)
	{
		_mouse = pos;
		if (_tree == nullptr)
			return;
		const widget_id_t target = hit_test(_mouse);
		fire_hover_change(target);
	}

	void input_router_t::on_mouse_button(mouse_button_e btn, bool pressed)
	{
		const u32 b = static_cast<u32>(btn);

		if (pressed)
		{
			widget_id_t target = _hovered;
			if (_popup_scope.active)
			{
				const widget_id_t raw_target = raw_hit_test(_mouse);
				if (!is_in_popup_scope(raw_target))
				{
					if (_popup_scope.on_outside_press)
						_popup_scope.on_outside_press(*this, _mouse, btn, _popup_scope.user_data);
					_pressed[b]		  = NULL_WIDGET;
					_pressed_state[b] = {};
					fire_hover_change(hit_test(_mouse));
					return;
				}
			}

			set_focus(find_focus_target(target), false);

			_pressed[b]		  = target;
			_pressed_state[b] = {target, _mouse, 0.0f, false};
			if (target != NULL_WIDGET)
			{
				auto it = _listeners.find(target);
				if (it != _listeners.end() && it->second.on_press)
					it->second.on_press(*this, target, _mouse, btn, it->second.user_data);
			}
			return;
		}

		const widget_id_t target = _pressed[b];
		press_state_t&	  ps	 = _pressed_state[b];
		_pressed[b]				 = NULL_WIDGET;

		if (target == NULL_WIDGET)
			return;

		auto					lit			 = _listeners.find(target);
		const bool				has_listener = lit != _listeners.end();
		const listener_bundle_t listener	 = has_listener ? lit->second : listener_bundle_t{};
		if (listener.on_release)
			listener.on_release(*this, target, _mouse, btn, listener.user_data);
		if (_tree != nullptr && !_tree->is_alive(target))
			return;

		if (ps.dragging)
		{
			if (listener.on_drag_end)
				listener.on_drag_end(*this, target, _mouse, _mouse - ps.press_pos, listener.user_data);
			return;
		}

		const widget_id_t under = hit_test(_mouse);
		if (under != target || ps.held_seconds > _config.click_max_seconds)
			return;

		if (listener.on_click)
			listener.on_click(*this, target, _mouse, btn, listener.user_data);
		if (_tree != nullptr && !_tree->is_alive(target))
			return;

		click_record_t& rec	  = _last_click[b];
		const f32		since = _accum_time - rec.t_seconds;
		if (rec.target == target && since <= _config.double_click_max_seconds)
		{
			lit = _listeners.find(target);
			if (lit != _listeners.end() && lit->second.on_double_click)
				lit->second.on_double_click(*this, target, _mouse, btn, lit->second.user_data);
			rec = {NULL_WIDGET, 0.0f};
		}
		else
		{
			rec = {target, _accum_time};
		}
	}

	void input_router_t::on_wheel(f32 delta)
	{
		widget_id_t cur = _hovered;
		while (cur != NULL_WIDGET)
		{
			auto it = _listeners.find(cur);
			if (it != _listeners.end() && it->second.on_wheel)
			{
				it->second.on_wheel(*this, cur, delta, it->second.user_data);
				return;
			}
			if (_tree == nullptr)
				return;
			cur = _tree->node(cur).parent;
		}
	}

	void input_router_t::on_key(const key_event_t& ev)
	{
		if (ev.action == key_action_e::press || ev.action == key_action_e::repeat)
		{
			if (ev.key == static_cast<u16>(input_code::key_tab))
			{
				if (ev.shift)
					prev_focus();
				else
					next_focus();
				return;
			}
			if (ev.key == static_cast<u16>(input_code::key_down))
			{
				next_focus();
				return;
			}
			if (ev.key == static_cast<u16>(input_code::key_up))
			{
				prev_focus();
				return;
			}
		}

		if (_focused == NULL_WIDGET)
			return;

		widget_id_t cur = _focused;
		while (cur != NULL_WIDGET)
		{
			auto it = _listeners.find(cur);
			if (it != _listeners.end() && it->second.on_key)
			{
				it->second.on_key(*this, cur, ev, it->second.user_data);
				return;
			}
			if (_tree == nullptr)
				return;
			cur = _tree->node(cur).parent;
		}
	}

	void input_router_t::set_focus(widget_id_t id, bool from_nav)
	{
		if (_focused == id)
			return;
		if (_focused != NULL_WIDGET)
		{
			auto it = _listeners.find(_focused);
			if (it != _listeners.end() && it->second.on_focus_lose)
				it->second.on_focus_lose(*this, _focused, from_nav, it->second.user_data);
		}
		_focused = id;
		if (_focused != NULL_WIDGET)
		{
			auto it = _listeners.find(_focused);
			if (it != _listeners.end() && it->second.on_focus_gain)
				it->second.on_focus_gain(*this, _focused, from_nav, it->second.user_data);
		}
	}

	void input_router_t::next_focus()
	{
		if (_focus_order.empty())
			return;
		size_t idx	 = 0;
		bool   found = false;
		if (_focused != NULL_WIDGET)
		{
			for (size_t i = 0; i < _focus_order.size(); ++i)
			{
				if (_focus_order[i] == _focused)
				{
					idx	  = (i + 1) % _focus_order.size();
					found = true;
					break;
				}
			}
		}

		const size_t start = found ? idx : 0;
		for (size_t i = 0; i < _focus_order.size(); ++i)
		{
			const widget_id_t candidate = _focus_order[(start + i) % _focus_order.size()];
			if (!_popup_scope.active || is_in_popup_scope(candidate))
			{
				set_focus(candidate, true);
				return;
			}
		}
	}

	void input_router_t::prev_focus()
	{
		if (_focus_order.empty())
			return;
		size_t idx	 = _focus_order.size() - 1;
		bool   found = false;
		if (_focused != NULL_WIDGET)
		{
			for (size_t i = 0; i < _focus_order.size(); ++i)
			{
				if (_focus_order[i] == _focused)
				{
					idx	  = (i + _focus_order.size() - 1) % _focus_order.size();
					found = true;
					break;
				}
			}
		}

		const size_t start = found ? idx : _focus_order.size() - 1;
		for (size_t i = 0; i < _focus_order.size(); ++i)
		{
			const widget_id_t candidate = _focus_order[(start + _focus_order.size() - i) % _focus_order.size()];
			if (!_popup_scope.active || is_in_popup_scope(candidate))
			{
				set_focus(candidate, true);
				return;
			}
		}
	}
}
