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

#ifdef JPH_DEBUG_RENDERER
#include <sfg/math/color.hpp>
#include <sfg/runtime/world/world_debug_draw.hpp>
#endif

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/FixedSizeFreeList.h>
#include <Jolt/Core/JobSystemWithBarrier.h>
#include <Jolt/Physics/PhysicsSettings.h>

#ifdef JPH_DEBUG_RENDERER
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Renderer/DebugRenderer.h>
#endif

namespace sfg
{
	namespace
	{
#ifdef JPH_DEBUG_RENDERER
		class physics_debug_renderer_t final : public JPH::DebugRenderer
		{
		public:
			physics_debug_renderer_t()
			{
				Initialize();
			}

			void DrawLine(JPH::RVec3Arg from, JPH::RVec3Arg to, JPH::ColorArg color) override
			{
				SFG_ASSERT(_debug_draw != nullptr);
				_debug_draw->draw_line(
					{static_cast<f32>(from.GetX()), static_cast<f32>(from.GetY()), static_cast<f32>(from.GetZ())}, {static_cast<f32>(to.GetX()), static_cast<f32>(to.GetY()), static_cast<f32>(to.GetZ())}, color_t::from255(color.r, color.g, color.b, color.a));
			}

			void DrawTriangle(JPH::RVec3Arg p0, JPH::RVec3Arg p1, JPH::RVec3Arg p2, JPH::ColorArg color, ECastShadow cast_shadow) override
			{
				SFG_ASSERT(_debug_draw != nullptr);
				_debug_draw->draw_triangle({static_cast<f32>(p0.GetX()), static_cast<f32>(p0.GetY()), static_cast<f32>(p0.GetZ())},
										   {static_cast<f32>(p1.GetX()), static_cast<f32>(p1.GetY()), static_cast<f32>(p1.GetZ())},
										   {static_cast<f32>(p2.GetX()), static_cast<f32>(p2.GetY()), static_cast<f32>(p2.GetZ())},
										   color_t::from255(color.r, color.g, color.b, color.a));
			}

			void DrawText3D(JPH::RVec3Arg position, const std::string_view& text, JPH::ColorArg color, f32 height) override
			{
			}

			Batch CreateTriangleBatch(const Triangle* triangles, int triangle_count) override
			{
				batch_t* batch = new batch_t;
				if (triangles != nullptr && triangle_count != 0)
					batch->triangles.assign(triangles, triangles + triangle_count);

				return batch;
			}

			Batch CreateTriangleBatch(const Vertex* vertices, int vertex_count, const JPH::uint32* indices, int index_count) override
			{
				batch_t* batch = new batch_t;
				if (vertices == nullptr || vertex_count == 0 || indices == nullptr || index_count == 0)
					return batch;

				batch->triangles.resize(index_count / 3);
				for (size_t i = 0; i < batch->triangles.size(); ++i)
				{
					Triangle& triangle = batch->triangles[i];
					triangle.mV[0]	   = vertices[indices[i * 3]];
					triangle.mV[1]	   = vertices[indices[i * 3 + 1]];
					triangle.mV[2]	   = vertices[indices[i * 3 + 2]];
				}

				return batch;
			}

			void DrawGeometry(JPH::RMat44Arg	 model_matrix,
							  const JPH::AABox&	 world_bounds,
							  f32				 lod_scale_sq,
							  JPH::ColorArg		 model_color,
							  const GeometryRef& geometry,
							  ECullMode			 cull_mode	 = ECullMode::CullBackFace,
							  ECastShadow		 cast_shadow = ECastShadow::On,
							  EDrawMode			 draw_mode	 = EDrawMode::Solid) override
			{
				const LOD&	   lod	 = geometry->mLODs.front();
				const batch_t* batch = static_cast<const batch_t*>(lod.mTriangleBatch.GetPtr());
				for (const Triangle& triangle : batch->triangles)
				{
					const JPH::RVec3 p0	   = model_matrix * JPH::Vec3(triangle.mV[0].mPosition);
					const JPH::RVec3 p1	   = model_matrix * JPH::Vec3(triangle.mV[1].mPosition);
					const JPH::RVec3 p2	   = model_matrix * JPH::Vec3(triangle.mV[2].mPosition);
					const JPH::Color color = model_color * triangle.mV[0].mColor;

					if (draw_mode == EDrawMode::Wireframe)
					{
						DrawLine(p0, p1, color);
						DrawLine(p1, p2, color);
						DrawLine(p2, p0, color);
					}
					else
					{
						DrawTriangle(p0, p1, p2, color, cast_shadow);
					}
				}
			}

			void draw(JPH::PhysicsSystem& system, world_debug_draw_t& debug_draw)
			{
				_debug_draw								= &debug_draw;
				JPH::BodyManager::DrawSettings settings = {};
				settings.mDrawShape						= true;
				settings.mDrawVelocity					= true;
				settings.mDrawMassAndInertia			= true;
				settings.mDrawSleepStats				= true;
				system.DrawBodies(settings, this);
				system.DrawConstraints(this);
				system.DrawConstraintLimits(this);
				_debug_draw = nullptr;
			}

		private:
			class batch_t final : public JPH::RefTargetVirtual
			{
			public:
				JPH_OVERRIDE_NEW_DELETE

				void AddRef() override
				{
					++_ref_count;
				}

				void Release() override
				{
					if (--_ref_count == 0)
						delete this;
				}

				JPH::Array<Triangle> triangles;

			private:
				JPH::atomic<JPH::uint32> _ref_count = 0;
			};

			world_debug_draw_t* _debug_draw = nullptr;
		};
#endif

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
#ifdef JPH_DEBUG_RENDERER
		physics_debug_renderer_t* g_physics_debug_renderer = nullptr;
#endif

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
		JPH_IF_DEBUG_RENDERER(g_physics_debug_renderer = new physics_debug_renderer_t();)
		g_physics_job_system = new physics_job_system_t();
	}

	void physics_runtime_t::uninit()
	{
		SFG_ASSERT(g_physics_job_system != nullptr);
		job_system_t::get().wait_for_all();
		delete g_physics_job_system;
		g_physics_job_system = nullptr;

		JPH_IF_DEBUG_RENDERER(delete g_physics_debug_renderer; g_physics_debug_renderer = nullptr;)
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

	void physics_runtime_t::draw_debug(JPH::PhysicsSystem& system, world_debug_draw_t& debug_draw)
	{
#ifdef JPH_DEBUG_RENDERER
		SFG_ASSERT(g_physics_debug_renderer != nullptr);
		g_physics_debug_renderer->draw(system, debug_draw);
#endif
	}
}
