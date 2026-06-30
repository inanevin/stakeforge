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
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/
#include "ui/editor_modal_cook_options.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widget_fold.hpp"
#include "ui/widgets/editor_widget_reflection.hpp"
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	void editor_modal_cook_options_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		_ui							= &ui;
		ui::layout_tree_t&	  tree	= ui.get_tree();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "modal_cook_options");
		tree.attach(parent, _root);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flags			 = ui::wf_visible;
		root_in.size_mode_x		 = ui::axis_mode_e::fixed;
		root_in.size_mode_y		 = ui::axis_mode_e::sum_children;
		root_in.size_value.x	 = theme.item_width * 4.0f;
		root_in.flow			 = ui::flow_e::column;
		root_in.child_spacing	 = theme.item_spacing;

		_folds.reserve(_options.size());
		_reflections.reserve(_options.size());
		for (editor_modal_cook_option_desc_t& option : _options)
		{
			editor_widget_fold_t* fold = new editor_widget_fold_t();
			fold->init(ui, _root, {.label = option.title, .folded = false});
			_folds.push_back(fold);

			editor_widget_reflection_t* reflection = new editor_widget_reflection_t();
			reflection->init(ui, fold->get_body(), {.objects = {.data = &option.object, .size = 1}, .type_id = option.type_id});
			_reflections.push_back(reflection);
		}
	}

	void editor_modal_cook_options_t::uninit()
	{
		for (editor_widget_reflection_t* reflection : _reflections)
		{
			reflection->uninit();
			delete reflection;
		}
		for (editor_widget_fold_t* fold : _folds)
		{
			fold->uninit();
			delete fold;
		}
		_reflections.resize(0);
		_folds.resize(0);
		_ui->deallocate_widget(_root);
		_ui	  = nullptr;
		_root = NULL_WIDGET;
	}

	void editor_modal_cook_options_t::set_options(const editor_modal_cook_option_desc_t* options, u16 count)
	{
		_options.resize(0);
		_options.reserve(count);
		for (u16 i = 0; i < count; ++i)
			_options.push_back(options[i]);
	}

	editor_modal_content_desc_t editor_modal_cook_options_t::get_content_desc()
	{
		return {.init = init_content, .uninit = uninit_content, .user_data = this};
	}

	void editor_modal_cook_options_t::init_content(ui::ui_context& ui, ui::widget_id_t parent, void* user_data)
	{
		static_cast<editor_modal_cook_options_t*>(user_data)->init(ui, parent);
	}

	void editor_modal_cook_options_t::uninit_content(void* user_data)
	{
		static_cast<editor_modal_cook_options_t*>(user_data)->uninit();
	}
}
