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

#include "memory_tracer.hpp"

#ifdef SFG_ENABLE_MEMORY_TRACER

#include "memory.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/platform/process.hpp>
#include <tracy/Tracy.hpp>
#include <fstream>
#include <sstream>
#include <Windows.h>
#include <DbgHelp.h>

#include <iostream>

#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "DbgHelp.lib")

namespace sfg
{
	u8 memory_tracer_t::s_category_counter = 0;

	namespace
	{
		thread_local vector_malloc<u8> s_category_stack;
		thread_local u8				   s_active_category_id = 0;

		void print(u32 n)
		{
			const size_t bufferSize = 256;
			char*		 buffer_t	= static_cast<char*>(malloc(bufferSize));
			if (!buffer_t)
				return;
			const int written = snprintf(buffer_t, bufferSize, " %d\n", n);

			if (written > 0 && static_cast<size_t>(written) < bufferSize)
			{
				WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), buffer_t, static_cast<DWORD>(written), NULL, NULL);
			}
		}
	}

	vector_malloc<u8>& memory_tracer_t::category_stack()
	{
		return s_category_stack;
	}

	u8& memory_tracer_t::active_category_id()
	{
		return s_active_category_id;
	}

	memory_category_t* memory_tracer_t::find_category(u8 id)
	{
		if (id == 0)
			return nullptr;

		for (memory_category_t& category : _categories)
		{
			if (category.id == id)
				return &category;
		}

		return nullptr;
	}

	const memory_category_t* memory_tracer_t::find_category(u8 id) const
	{
		if (id == 0)
			return nullptr;

		for (const memory_category_t& category : _categories)
		{
			if (category.id == id)
				return &category;
		}

		return nullptr;
	}

	void memory_tracer_t::on_allocation(void* ptr, size_t sz)
	{
		if (ptr == nullptr)
			return;

		LOCK_GUARD(_category_mtx);

		SFG_ASSERT(_allocations.find(ptr) == _allocations.end());
		memory_track_t& track = _allocations[ptr];
		track.ptr			  = ptr;
		track.size			  = sz;
		track.category_id	  = active_category_id();
		capture_trace(track);

		memory_category_t* cat = find_category(track.category_id);
		if (cat != nullptr)
		{
			cat->total_size += track.size;
			TracyAllocN(ptr, sz, cat->name);
		}
		else
		{
			TracyAlloc(ptr, sz);
		}
	}

	void memory_tracer_t::on_free(void* ptr)
	{
		if (ptr == nullptr)
			return;

		LOCK_GUARD(_category_mtx);

		auto it = _allocations.find(ptr);
		if (it == _allocations.end())
		{
			// TracyFree(ptr);
			return;
		}

		const memory_track_t track = it->second;
		if (memory_category_t* cat = find_category(track.category_id))
		{
			SFG_ASSERT(cat->total_size >= track.size);
			cat->total_size -= track.size;
			TracyFreeN(ptr, cat->name);
		}
		else
		{
			TracyFree(ptr);
		}

		_allocations.erase(it);
	}

	void memory_tracer_t::push_category(const char* name)
	{
		LOCK_GUARD(_category_mtx);

		vector_malloc<u8>& stack = category_stack();

		auto it = std::find_if(_categories.begin(), _categories.end(), [&](const memory_category_t& saved) -> bool { return strcmp(saved.name, name) == 0; });
		if (it != _categories.end())
		{
			active_category_id() = it->id;
			stack.push_back(active_category_id());
			return;
		}

		memory_category_t cat = {};
		const size_t	  sz  = strlen(name) + 1;
		cat.name			  = reinterpret_cast<const char*>(malloc(sz));

		if (cat.name)
			SFG_MEMCPY((void*)cat.name, (void*)name, sz);
		cat.id = ++s_category_counter;
		stack.push_back(cat.id);
		active_category_id() = cat.id;
		_categories.push_back(cat);
	}

	void memory_tracer_t::pop_category()
	{
		vector_malloc<u8>& stack = category_stack();
		SFG_ASSERT(!stack.empty());

		stack.pop_back();

		if (!stack.empty())
		{
			active_category_id() = stack.back();
			return;
		}

		active_category_id() = 0;
	}

	void memory_tracer_t::capture_trace(memory_track_t& track)
	{
		track.stack_size = CaptureStackBackTrace(3, MEMORY_STACK_TRACE_SIZE, track.stack, nullptr);
	}

	void memory_tracer_t::destroy()
	{
		HANDLE process = GetCurrentProcess();
		SymCleanup(process);

		for (const memory_category_t& cat : _categories)
			free((void*)cat.name);

		check_leaks();
	}

	void memory_tracer_t::check_leaks()
	{
		std::ostringstream leak_stream;
		size_t			   total_leak = 0;

		for (auto& [ptr, alloc] : _allocations)
		{
			total_leak += alloc.size;

			HANDLE		process = GetCurrentProcess();
			static bool inited	= false;

			if (!inited)
			{
				inited = true;
				SymInitialize(process, nullptr, TRUE);
			}

			void* symbolAll = calloc(sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR), 1);

			if (symbolAll == NULL)
				return;

			SYMBOL_INFO* symbol	 = static_cast<SYMBOL_INFO*>(symbolAll);
			symbol->MaxNameLen	 = 255;
			symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

			DWORD			 displacement;
			IMAGEHLP_LINE64* line = NULL;
			line				  = (IMAGEHLP_LINE64*)std::malloc(sizeof(IMAGEHLP_LINE64));

			if (line == NULL)
				return;

			line->SizeOfStruct = sizeof(IMAGEHLP_LINE64);

			bool not_valid = false;

			for (int i = 0; i < alloc.stack_size; ++i)
			{
				leak_stream << "------ Stack Trace " << i << "------\n";

				DWORD64 address = (DWORD64)(alloc.stack[i]);

				SymFromAddr(process, address, NULL, symbol);

				if (SymGetLineFromAddr64(process, address, &displacement, line))
				{
					const string_t fn = line->FileName;

					leak_stream << "Location:" << line->FileName << "\n";
					leak_stream << "Smybol:" << symbol->Name << "\n";
					leak_stream << "Line:" << line->LineNumber << "\n";
					leak_stream << "SymbolAddr:" << symbol->Address << "\n";
				}
				else
				{
					leak_stream << "Smybol:" << symbol->Name << "\n";
					leak_stream << "SymbolAddr:" << symbol->Address << "\n";
				}

				IMAGEHLP_MODULE64 moduleInfo;
				moduleInfo.SizeOfStruct = sizeof(moduleInfo);
				if (::SymGetModuleInfo64(process, symbol->ModBase, &moduleInfo))
					leak_stream << "Module:" << moduleInfo.ModuleName << "\n";
			}

			if (not_valid)
				continue;

			std::free(line);
			std::free(symbolAll);

			leak_stream << "\n";
			leak_stream << "\n";
		}

		if (total_leak != 0)
		{
			std::ostringstream head;
			head << "Total: " << total_leak << " bytes, " << (static_cast<double>(total_leak) / (double)(1024 * 1024)) << " mbs \n";
			head << leak_stream.str();

			process::message_box("Leak detected!", head.str().c_str());
			leak_stream.clear();
			head.clear();
		}
	}
}

#endif
