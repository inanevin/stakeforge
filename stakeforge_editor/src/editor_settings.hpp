// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/common/size_definitions.hpp>
#include <sfg/data/span.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/math/vec2i16.hpp>
#include <sfg/math/vec2u16.hpp>
#include <sfg/vendor/nhlohmann/json_fwd.hpp>

namespace sfg
{
	struct editor_window_settings_t
	{
		vec2i16_t position		= {64, 64};
		vec2u16_t size			= {1280, 720};
		u64		  monitor_ident = UINT64_MAX;
	};

	class editor_settings_t
	{
	public:
		inline static editor_settings_t& get()
		{
			static editor_settings_t instance;
			return instance;
		}

		bool reload();
		void save();

		u16	 add_window(const editor_window_settings_t& w);
		void remove_window(u16 index);

		inline editor_window_settings_t& get_window(u16 index)
		{
			return _windows[index];
		}

		inline const editor_window_settings_t& get_window(u16 index) const
		{
			return _windows[index];
		}

		inline span_t<const editor_window_settings_t> get_windows() const
		{
			return {_windows.data(), _windows.size()};
		}

		inline u16 get_window_count() const
		{
			return static_cast<u16>(_windows.size());
		}

	private:
		void flush_to_disk();

		editor_settings_t()									   = default;
		~editor_settings_t()								   = default;
		editor_settings_t(const editor_settings_t&)			   = delete;
		editor_settings_t& operator=(const editor_settings_t&) = delete;

		vector_t<editor_window_settings_t> _windows;

		friend void to_json(nlohmann::json& j, const editor_settings_t& settings);
		friend void from_json(const nlohmann::json& j, editor_settings_t& settings);
	};

	void to_json(nlohmann::json& j, const editor_window_settings_t& settings);
	void from_json(const nlohmann::json& j, editor_window_settings_t& settings);
	void to_json(nlohmann::json& j, const editor_settings_t& settings);
	void from_json(const nlohmann::json& j, editor_settings_t& settings);
}
