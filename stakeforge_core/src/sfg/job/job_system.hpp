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

#include <sfg/common/size_definitions.hpp>
#include <sfg/data/unique.hpp>
#include <sfg/vendor/taskflow/taskflow.hpp>

namespace sfg
{
	using job_graph_t						 = tf::Taskflow;
	using job_task_t						 = tf::Task;
	using job_async_handle_t				 = tf::AsyncTask;
	template <typename T> using job_future_t = tf::Future<T>;

	class job_system_t
	{
	public:
		static job_system_t& get()
		{
			static job_system_t instance;
			return instance;
		}

		void init(u32 worker_count = 0);
		void uninit();

		bool is_initialized() const
		{
			return _executor != nullptr;
		}

		job_future_t<void> run(job_graph_t& graph)
		{
			return _executor->run(graph);
		}

		job_future_t<void> run(job_graph_t&& graph)
		{
			return _executor->run(std::move(graph));
		}

		template <typename C> job_future_t<void> run(job_graph_t& graph, C&& callable)
		{
			return _executor->run(graph, std::forward<C>(callable));
		}

		job_future_t<void> run_n(job_graph_t& graph, u32 n)
		{
			return _executor->run_n(graph, static_cast<size_t>(n));
		}

		template <typename F> auto async(F&& func)
		{
			return _executor->async(std::forward<F>(func));
		}

		template <typename F> void silent_async(F&& func)
		{
			_executor->silent_async(std::forward<F>(func));
		}

		template <typename F, typename... Tasks> auto dependent_async(F&& func, Tasks&&... tasks)
		{
			return _executor->dependent_async(std::forward<F>(func), std::forward<Tasks>(tasks)...);
		}

		template <typename F, typename... Tasks> job_async_handle_t silent_dependent_async(F&& func, Tasks&&... tasks)
		{
			return _executor->silent_dependent_async(std::forward<F>(func), std::forward<Tasks>(tasks)...);
		}

		void wait_for_all()
		{
			_executor->wait_for_all();
		}

		u32 get_worker_count() const
		{
			return static_cast<u32>(_executor->num_workers());
		}

		i32 get_this_worker_id() const
		{
			return _executor->this_worker_id();
		}

		tf::Executor& get_executor()
		{
			return *_executor;
		}

	private:
		job_system_t() = default;
		~job_system_t();

		job_system_t(const job_system_t&)			 = delete;
		job_system_t& operator=(const job_system_t&) = delete;

	private:
		unique_t<tf::Executor> _executor;
	};
}
