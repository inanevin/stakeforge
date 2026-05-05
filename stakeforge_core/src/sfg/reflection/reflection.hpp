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

#pragma once
#include "common_reflection.hpp"
#include <sfg/data/vector.hpp>
#include <sfg/data/string.hpp>
#include <sfg/common/hashing.hpp>
#include <sfg/common/type_id.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/memory/malloc_allocator_stl.hpp>
#include <sfg/memory/memory.hpp>
#include <functional>

#pragma warning(push)
#pragma warning(disable : 4541)

namespace sfg
{
	using malloc_string = std::basic_string<char, std::char_traits<char>, malloc_allocator_stl_t<char>>;

	class field_value_t
	{
	public:
		field_value_t() {};
		field_value_t(void* addr) : _ptr(addr) {};
		template <typename T> T get_value()
		{
			return cast<T>();
		}

		template <typename T> void set_value(T t)
		{
			cast<T>() = t;
		}

		template <typename T> T cast()
		{
			return *static_cast<T*>(_ptr);
		}

		template <typename T> T& cast_ref()
		{
			return *static_cast<T*>(_ptr);
		}

		template <typename T> T* cast_ptr()
		{
			return static_cast<T*>(_ptr);
		}

		void* get_ptr() const
		{
			return _ptr;
		}

	private:
		template <typename T, typename U> friend class field_t;
		void* _ptr = nullptr;
	};

	class field_base_t
	{
	public:
		typedef vector_t<malloc_string, malloc_allocator_stl_t<malloc_string>> enum_vec;

		field_base_t()			= default;
		virtual ~field_base_t() = default;

		virtual field_value_t value(void* obj) const = 0;
		virtual size_t		  get_type_size() const	 = 0;

		enum_vec			 _enum_list	  = {};
		malloc_string		 _title		  = "";
		malloc_string		 _tooltip	  = "";
		sid_t				 _sid		  = 0;
		sid_t				 _sub_type_id = 0;
		f32					 _min		  = 0.0f;
		f32					 _max		  = 0.0f;
		reflected_field_type _type		  = reflected_field_type::rf_float;
		u8					 _is_list	  = 0;
		u8					 _no_ui		  = 0;
		u8					 _clamped	  = 0;
	};

	template <typename T, class C> class field_t : public field_base_t
	{
	public:
		field_t()		   = default;
		virtual ~field_t() = default;

		virtual size_t get_type_size() const override
		{
			return sizeof(T);
		}

		inline virtual field_value_t value(void* obj) const override
		{
			field_value_t val;
			val._ptr = &((static_cast<C*>(obj))->*(_var));
			return val;
		}

		T _var = T();
	};

	struct reflection_function_base_t
	{
		virtual ~reflection_function_base_t() = default;
		virtual size_t signature_hash() const = 0;
	};

	template <typename RetVal, typename... Args> struct reflection_function_t : public reflection_function_base_t
	{
		using FuncType = std::function<RetVal(Args...)>;
		FuncType func;

		reflection_function_t(FuncType f) : func(std::move(f))
		{
		}

		size_t signature_hash() const override
		{
			return typeid(reflection_function_t<RetVal, Args...>).hash_code();
		}

		RetVal invoke(Args... args)
		{
			return func(std::forward<Args>(args)...);
		}
	};

	struct control_button_t
	{
		malloc_string title	  = "";
		malloc_string tooltip = "";
		sid_t		  sid	  = 0;
	};

	class meta_t
	{
	public:
		struct function_entry_t
		{
			sid_t						id	= 0;
			reflection_function_base_t* ptr = nullptr;
		};

		typedef vector_t<function_entry_t, malloc_allocator_stl_t<function_entry_t>> function_vec;
		typedef vector_t<field_base_t*, malloc_allocator_stl_t<field_base_t*>>		 field_vec;
		typedef vector_t<control_button_t, malloc_allocator_stl_t<control_button_t>> button_vec;

		template <auto DATA, typename Class> field_base_t* add_field(const string_t& title, reflected_field_type type, const string_t& tooltip, f32 min, f32 max, sid_t sub_type_id = 0, u8 is_list = 0, u8 no_ui = 0)
		{
			using ft = field_t<decltype(DATA), Class>;

			void* mem		= SFG_ALIGNED_MALLOC(alignof(ft), sizeof(ft));
			ft*	  f			= new (mem) ft();
			f->_var			= DATA;
			f->_sid			= TO_SID(title);
			f->_type		= type;
			f->_min			= min;
			f->_max			= max;
			f->_sub_type_id = sub_type_id;
			f->_tooltip		= tooltip;
			f->_title		= title;
			f->_clamped		= 1;
			f->_is_list		= is_list;
			f->_no_ui		= no_ui;
			_fields.push_back(f);
			return f;
		}

		template <auto DATA, typename Class> field_base_t* add_field(const string_t& title, reflected_field_type type, const string_t& tooltip, sid_t sub_type_id = 0, u8 is_list = 0, u8 no_ui = 0)
		{
			using ft = field_t<decltype(DATA), Class>;

			void* mem		= SFG_ALIGNED_MALLOC(alignof(ft), sizeof(ft));
			ft*	  f			= new (mem) ft();
			f->_var			= DATA;
			f->_sid			= TO_SID(title);
			f->_type		= type;
			f->_min			= 0.0f;
			f->_max			= 0.0f;
			f->_sub_type_id = sub_type_id;
			f->_tooltip		= tooltip;
			f->_title		= title;
			f->_is_list		= is_list;
			f->_no_ui		= no_ui;
			_fields.push_back(f);
			return f;
		}

		void add_control_button(const string_t& title, const string_t& tooltip)
		{
			_control_buttons.push_back({.title = title.c_str(), .tooltip = tooltip.c_str(), .sid = TO_SID(title)});
		}

		template <typename RetVal, typename... Args, typename F> meta_t& add_function(sid_t id, F&& f)
		{
			using func_t						= reflection_function_t<RetVal, Args...>;
			std::function<RetVal(Args...)> func = std::forward<F>(f);

			void*			  mem			 = SFG_ALIGNED_MALLOC(alignof(func_t), sizeof(func_t));
			func_t*			  reflectionFunc = new (mem) func_t(std::move(func));
			function_entry_t* entry_t		 = find_function_entry(id);
			if (entry_t)
				entry_t->ptr = reflectionFunc;
			else
				_functions.push_back({.id = id, .ptr = reflectionFunc});

			//_functions[id]						= std::make_unique<func_t>(std::move(func));
			return *this;
		}

		template <typename RetVal, typename... Args> RetVal invoke_function(sid_t id, Args... args) const
		{
			const function_entry_t* entry_t = find_function_entry(id);
			auto*					ptr		= entry_t->ptr;
			// Signature check
			// if (ptr->signature_hash() != typeid(reflection_function<RetVal, Args...>).hash_code())
			// 	throw std::runtime_error("Signature mismatch");

			auto* func = static_cast<reflection_function_t<RetVal, Args...>*>(ptr);
			return func->invoke(std::forward<Args>(args)...);
		}

		reflection_function_base_t* get_function(sid_t id)
		{
			function_entry_t* entry_t = find_function_entry(id);
			return entry_t ? entry_t->ptr : nullptr;
		}

		bool has_function(sid_t id) const
		{
			return find_function_entry(id) != nullptr;
		}

		inline sid_t get_type_id() const
		{
			return _type_id;
		}

		inline const sid_t get_tag() const
		{
			return _tag;
		}

		inline const malloc_string& get_tag_str() const
		{
			return _tag_str;
		}

		inline u32 get_type_index() const
		{
			return _type_index;
		}

		inline const field_vec& get_fields() const
		{
			return _fields;
		}

		inline void set_title(const char* t)
		{
			_title = t;
		}

		inline void set_category(const char* t)
		{
			_category = t;
		}

		inline const malloc_string& get_title() const
		{
			return _title;
		}

		inline const malloc_string& get_category() const
		{
			return _category;
		}

		inline const button_vec& get_control_buttons() const
		{
			return _control_buttons;
		}

	private:
		friend class reflection_t;

		inline void destroy()
		{
			for (auto& entry_t : _functions)
			{
				if (entry_t.ptr)
				{
					entry_t.ptr->~reflection_function_base_t();
					SFG_ALIGNED_FREE(entry_t.ptr);
				}
			}

			for (auto f : _fields)
			{
				SFG_ALIGNED_FREE(f);
			}
		}

	private:
		inline function_entry_t* find_function_entry(sid_t id)
		{
			for (function_entry_t& entry_t : _functions)
			{
				if (entry_t.id == id)
					return &entry_t;
			}
			return nullptr;
		}

		inline const function_entry_t* find_function_entry(sid_t id) const
		{
			for (const function_entry_t& entry_t : _functions)
			{
				if (entry_t.id == id)
					return &entry_t;
			}
			return nullptr;
		}

		function_vec  _functions;
		field_vec	  _fields;
		button_vec	  _control_buttons = {};
		malloc_string _title		   = "";
		malloc_string _category		   = "";
		malloc_string _tag_str		   = "";
		sid_t		  _type_id		   = 0;
		sid_t		  _tag			   = 0;
		u32			  _type_index	   = 0;
	};

	class reflection_t
	{
	public:
		struct meta_entry_t
		{
			sid_t  id = 0;
			meta_t meta_t;
		};

		typedef vector_t<meta_entry_t, malloc_allocator_stl_t<meta_entry_t>> meta_vec;

		static reflection_t& get()
		{
			static reflection_t ref;
			return ref;
		}

		~reflection_t()
		{
			for (auto& entry_t : _metas)
			{
				entry_t.meta_t.destroy();
			}
		}

		meta_t& register_meta(sid_t id, u32 index, const string_t& tag)
		{
			meta_entry_t* entry_t = find_meta_entry(id);
			if (!entry_t)
			{
				_metas.push_back({.id = id});
				entry_t = &_metas.back();
			}

			meta_t& m	  = entry_t->meta_t;
			m._type_id	  = id;
			m._tag		  = TO_SID(tag);
			m._tag_str	  = tag;
			m._type_index = index;
			return m;
		}

		meta_t& resolve(sid_t id)
		{
			meta_entry_t* entry_t = find_meta_entry(id);
			return entry_t->meta_t;
		}

		meta_t* try_resolve(sid_t id)
		{
			meta_entry_t* entry_t = find_meta_entry(id);
			return entry_t ? &entry_t->meta_t : nullptr;
		}

		const meta_vec& get_metas() const
		{
			return _metas;
		}

		const meta_t* find_by_tag(const char* tag) const;

	private:
		inline meta_entry_t* find_meta_entry(sid_t id)
		{
			for (meta_entry_t& entry_t : _metas)
			{
				if (entry_t.id == id)
					return &entry_t;
			}
			return nullptr;
		}

		inline const meta_entry_t* find_meta_entry(sid_t id) const
		{
			for (const meta_entry_t& entry_t : _metas)
			{
				if (entry_t.id == id)
					return &entry_t;
			}
			return nullptr;
		}

		meta_vec _metas;
	};

#define SFG_PP_CONCAT_INNER(a, b) a##b
#define SFG_PP_CONCAT(a, b)		  SFG_PP_CONCAT_INNER(a, b)

	/*
#define REFLECT_FIELD(CLASSNAME, FIELDNAME, TITLE, FIELD_TYPE, TOOLTIP, MIN, MAX)                                                                                                                                                                                  \
	struct reflected_field_##CLASSNAME##FIELDNAME                                                                                                                                                                                                                  \
	{                                                                                                                                                                                                                                                              \
		reflected_field_##CLASSNAME##FIELDNAME()                                                                                                                                                                                                                   \
		{                                                                                                                                                                                                                                                          \
			reflection::get().resolve(type_id<CLASSNAME>::value).add_field<&CLASSNAME::FIELDNAME, CLASSNAME>(TITLE##_hs, FIELD_TYPE, TITLE, TOOLTIP, MIN, MAX);                                                                                                    \
		}                                                                                                                                                                                                                                                          \
	};                                                                                                                                                                                                                                                             \
	inline static reflected_field_##CLASSNAME##FIELDNAME SFG_PP_CONCAT(_ref_inst, __COUNTER__) = reflected_field_##CLASSNAME##FIELDNAME()
	*/

};

#pragma warning(pop)
