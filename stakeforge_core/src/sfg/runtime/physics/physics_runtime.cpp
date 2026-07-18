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

#include "physics_runtime.hpp"

#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/job/job_system.hpp>

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/FixedSizeFreeList.h>
#include <Jolt/Core/JobSystemWithBarrier.h>
#include <Jolt/Physics/PhysicsSettings.h>

namespace sfg
{
	namespace
	{
		class physics_job_system_t final : public JPH::JobSystemWithBarrier
		{
		public:
			physics_job_system_t()
			{
				Init(JPH::cMaxPhysicsBarriers);
				_jobs.Init(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsJobs);
			}

			int GetMaxConcurrency() const override
			{
				return static_cast<int>(job_system_t::get().get_worker_count()) + 1;
			}

			JobHandle CreateJob(const char* name, JPH::ColorArg color, const JobFunction& function, JPH::uint32 dependency_count = 0) override
			{
				const JPH::uint32 index = _jobs.ConstructObject(name, color, this, function, dependency_count);
				SFG_ASSERT(index != job_pool_t::cInvalidObjectIndex);

				Job* const job = &_jobs.Get(index);
				JobHandle  handle(job);

				if (dependency_count == 0)
					QueueJob(job);

				return handle;
			}

		protected:
			void QueueJob(Job* job) override
			{
				job->AddRef();
				job_system_t::get().silent_async([job]() {
					job->Execute();
					job->Release();
				});
			}

			void QueueJobs(Job** jobs, JPH::uint job_count) override
			{
				for (JPH::uint i = 0; i < job_count; ++i)
					QueueJob(jobs[i]);
			}

			void FreeJob(Job* job) override
			{
				_jobs.DestructObject(job);
			}

		private:
			using job_pool_t = JPH::FixedSizeFreeList<Job>;
			job_pool_t _jobs;
		};

		physics_job_system_t* g_physics_job_system = nullptr;

		void trace_impl(const char* format, ...)
		{
			char	buffer[4096] = {};
			va_list args;
			va_start(args, format);
			vsnprintf(buffer, sizeof(buffer), format, args);
			va_end(args);
			SFG_INFO("{0}", buffer);
		}

#ifdef JPH_ENABLE_ASSERTS
		bool assert_impl(const char* expression, const char* message, const char* file, JPH::uint line)
		{
			SFG_ERR("Jolt assertion: {0} ({1}:{2}) {3}", expression, file, line, message == nullptr ? "" : message);
			SFG_ASSERT(false);
			return true;
		}
#endif
	}

	void physics_runtime_t::init()
	{
		SFG_ASSERT(g_physics_job_system == nullptr);
		SFG_ASSERT(job_system_t::get().is_initialized());

		JPH::RegisterDefaultAllocator();
		JPH::Trace = trace_impl;
		JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = assert_impl;)
		JPH::Factory::sInstance = new JPH::Factory();
		JPH::RegisterTypes();
		g_physics_job_system = new physics_job_system_t();
	}

	void physics_runtime_t::uninit()
	{
		SFG_ASSERT(g_physics_job_system != nullptr);
		job_system_t::get().wait_for_all();
		delete g_physics_job_system;
		g_physics_job_system = nullptr;
		JPH::UnregisterTypes();
		delete JPH::Factory::sInstance;
		JPH::Factory::sInstance = nullptr;
		JPH::Trace				= nullptr;
	}

	JPH::JobSystem& physics_runtime_t::get_job_system()
	{
		SFG_ASSERT(g_physics_job_system != nullptr);
		return *g_physics_job_system;
	}
}
