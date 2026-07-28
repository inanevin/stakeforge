using System;

namespace SFG;

public static unsafe class Physics
{
    public static bool SetBodyLinearVelocity(World world, Entity entity, Vector3 velocity)
    {
        return ManagedRuntime.GetApi()->Physics->SetBodyLinearVelocity(world.GetNative(), entity.Id, &velocity) != 0;
    }

    public static bool SetBodyAngularVelocity(World world, Entity entity, Vector3 velocity)
    {
        return ManagedRuntime.GetApi()->Physics->SetBodyAngularVelocity(world.GetNative(), entity.Id, &velocity) != 0;
    }

    public static bool AddBodyForce(World world, Entity entity, Vector3 force)
    {
        return ManagedRuntime.GetApi()->Physics->AddBodyForce(world.GetNative(), entity.Id, &force) != 0;
    }

    public static bool AddBodyImpulse(World world, Entity entity, Vector3 impulse)
    {
        return ManagedRuntime.GetApi()->Physics->AddBodyImpulse(world.GetNative(), entity.Id, &impulse) != 0;
    }

    public static bool WakeBody(World world, Entity entity)
    {
        return ManagedRuntime.GetApi()->Physics->WakeBody(world.GetNative(), entity.Id) != 0;
    }

    public static bool TryGetBodyState(World world, Entity entity, out PhysicsBodyState state)
    {
        state = default;

        fixed (PhysicsBodyState* statePointer = &state)
        {
            return ManagedRuntime.GetApi()->Physics->GetBodyState(world.GetNative(), entity.Id, statePointer) != 0;
        }
    }

    public static bool RaycastAny(World world, in PhysicsRaycast ray)
    {
        return RaycastAny(world, ray, null);
    }

    public static bool RaycastAny(World world, in PhysicsRaycast ray, in PhysicsQueryFilter filter)
    {
        fixed (PhysicsQueryFilter* filterPointer = &filter)
        {
            return RaycastAny(world, ray, filterPointer);
        }
    }

    public static bool RaycastClosest(World world, in PhysicsRaycast ray, out PhysicsHit hit)
    {
        return RaycastClosest(world, ray, null, out hit);
    }

    public static bool RaycastClosest(World world, in PhysicsRaycast ray, in PhysicsQueryFilter filter, out PhysicsHit hit)
    {
        fixed (PhysicsQueryFilter* filterPointer = &filter)
        {
            return RaycastClosest(world, ray, filterPointer, out hit);
        }
    }

    public static bool TryRaycastAll(World world, in PhysicsRaycast ray, Span<PhysicsHit> hits, out PhysicsQueryResult result)
    {
        return TryRaycastAll(world, ray, null, hits, out result);
    }

    public static bool TryRaycastAll(World world, in PhysicsRaycast ray, in PhysicsQueryFilter filter, Span<PhysicsHit> hits, out PhysicsQueryResult result)
    {
        fixed (PhysicsQueryFilter* filterPointer = &filter)
        {
            return TryRaycastAll(world, ray, filterPointer, hits, out result);
        }
    }

    public static bool LinecastClosest(World world, in PhysicsLinecast line, out PhysicsHit hit)
    {
        return LinecastClosest(world, line, null, out hit);
    }

    public static bool LinecastClosest(World world, in PhysicsLinecast line, in PhysicsQueryFilter filter, out PhysicsHit hit)
    {
        fixed (PhysicsQueryFilter* filterPointer = &filter)
        {
            return LinecastClosest(world, line, filterPointer, out hit);
        }
    }

    public static void LinecastClosestBatch(World world, ReadOnlySpan<PhysicsLinecast> lines, Span<PhysicsHit> hits)
    {
        LinecastClosestBatch(world, lines, hits, null);
    }

    public static void LinecastClosestBatch(World world, ReadOnlySpan<PhysicsLinecast> lines, Span<PhysicsHit> hits, in PhysicsQueryFilter filter)
    {
        fixed (PhysicsQueryFilter* filterPointer = &filter)
        {
            LinecastClosestBatch(world, lines, hits, filterPointer);
        }
    }

    public static bool SpherecastAny(World world, in PhysicsSpherecast sphere)
    {
        return SpherecastAny(world, sphere, null);
    }

    public static bool SpherecastAny(World world, in PhysicsSpherecast sphere, in PhysicsQueryFilter filter)
    {
        fixed (PhysicsQueryFilter* filterPointer = &filter)
        {
            return SpherecastAny(world, sphere, filterPointer);
        }
    }

    public static bool SpherecastClosest(World world, in PhysicsSpherecast sphere, out PhysicsHit hit)
    {
        return SpherecastClosest(world, sphere, null, out hit);
    }

    public static bool SpherecastClosest(World world, in PhysicsSpherecast sphere, in PhysicsQueryFilter filter, out PhysicsHit hit)
    {
        fixed (PhysicsQueryFilter* filterPointer = &filter)
        {
            return SpherecastClosest(world, sphere, filterPointer, out hit);
        }
    }

    public static bool TrySpherecastAll(World world, in PhysicsSpherecast sphere, Span<PhysicsHit> hits, out PhysicsQueryResult result)
    {
        return TrySpherecastAll(world, sphere, null, hits, out result);
    }

    public static bool TrySpherecastAll(World world, in PhysicsSpherecast sphere, in PhysicsQueryFilter filter, Span<PhysicsHit> hits, out PhysicsQueryResult result)
    {
        fixed (PhysicsQueryFilter* filterPointer = &filter)
        {
            return TrySpherecastAll(world, sphere, filterPointer, hits, out result);
        }
    }

    public static bool SetCharacterVelocity(World world, Entity entity, Vector3 velocity)
    {
        return ManagedRuntime.GetApi()->Physics->SetCharacterVelocity(world.GetNative(), entity.Id, &velocity) != 0;
    }

    public static bool AddCharacterVelocity(World world, Entity entity, Vector3 velocity)
    {
        return ManagedRuntime.GetApi()->Physics->AddCharacterVelocity(world.GetNative(), entity.Id, &velocity) != 0;
    }

    public static bool JumpCharacter(World world, Entity entity, float speed)
    {
        return ManagedRuntime.GetApi()->Physics->JumpCharacter(world.GetNative(), entity.Id, speed) != 0;
    }

    public static bool TeleportCharacter(World world, Entity entity, Vector3 position)
    {
        return ManagedRuntime.GetApi()->Physics->TeleportCharacter(world.GetNative(), entity.Id, &position) != 0;
    }

    public static bool TryGetCharacterState(World world, Entity entity, out CharacterMoverState state)
    {
        state = default;

        fixed (CharacterMoverState* statePointer = &state)
        {
            return ManagedRuntime.GetApi()->Physics->GetCharacterState(world.GetNative(), entity.Id, statePointer) != 0;
        }
    }

    private static bool RaycastAny(World world, in PhysicsRaycast ray, PhysicsQueryFilter* filter)
    {
        fixed (PhysicsRaycast* rayPointer = &ray)
        {
            return ManagedRuntime.GetApi()->Physics->RaycastAny(world.GetNative(), rayPointer, filter) != 0;
        }
    }

    private static bool RaycastClosest(World world, in PhysicsRaycast ray, PhysicsQueryFilter* filter, out PhysicsHit hit)
    {
        hit = default;

        fixed (PhysicsRaycast* rayPointer = &ray)
        fixed (PhysicsHit* hitPointer = &hit)
        {
            return ManagedRuntime.GetApi()->Physics->RaycastClosest(world.GetNative(), rayPointer, filter, hitPointer) != 0;
        }
    }

    private static bool TryRaycastAll(World world, in PhysicsRaycast ray, PhysicsQueryFilter* filter, Span<PhysicsHit> hits, out PhysicsQueryResult result)
    {
        result = default;

        fixed (PhysicsRaycast* rayPointer = &ray)
        fixed (PhysicsHit* hitsPointer = hits)
        fixed (PhysicsQueryResult* resultPointer = &result)
        {
            return ManagedRuntime.GetApi()->Physics->RaycastAll(world.GetNative(), rayPointer, filter, hitsPointer, (uint)hits.Length, resultPointer) != 0;
        }
    }

    private static bool LinecastClosest(World world, in PhysicsLinecast line, PhysicsQueryFilter* filter, out PhysicsHit hit)
    {
        hit = default;

        fixed (PhysicsLinecast* linePointer = &line)
        fixed (PhysicsHit* hitPointer = &hit)
        {
            return ManagedRuntime.GetApi()->Physics->LinecastClosest(world.GetNative(), linePointer, filter, hitPointer) != 0;
        }
    }

    private static void LinecastClosestBatch(World world, ReadOnlySpan<PhysicsLinecast> lines, Span<PhysicsHit> hits, PhysicsQueryFilter* filter)
    {
        if (lines.Length != hits.Length)
        {
            throw new ArgumentException("The line and hit spans must have equal lengths.");
        }

        fixed (PhysicsLinecast* linesPointer = lines)
        fixed (PhysicsHit* hitsPointer = hits)
        {
            ManagedRuntime.GetApi()->Physics->LinecastClosestBatch(world.GetNative(), linesPointer, hitsPointer, (uint)lines.Length, filter);
        }
    }

    private static bool SpherecastAny(World world, in PhysicsSpherecast sphere, PhysicsQueryFilter* filter)
    {
        fixed (PhysicsSpherecast* spherePointer = &sphere)
        {
            return ManagedRuntime.GetApi()->Physics->SpherecastAny(world.GetNative(), spherePointer, filter) != 0;
        }
    }

    private static bool SpherecastClosest(World world, in PhysicsSpherecast sphere, PhysicsQueryFilter* filter, out PhysicsHit hit)
    {
        hit = default;

        fixed (PhysicsSpherecast* spherePointer = &sphere)
        fixed (PhysicsHit* hitPointer = &hit)
        {
            return ManagedRuntime.GetApi()->Physics->SpherecastClosest(world.GetNative(), spherePointer, filter, hitPointer) != 0;
        }
    }

    private static bool TrySpherecastAll(World world, in PhysicsSpherecast sphere, PhysicsQueryFilter* filter, Span<PhysicsHit> hits, out PhysicsQueryResult result)
    {
        result = default;

        fixed (PhysicsSpherecast* spherePointer = &sphere)
        fixed (PhysicsHit* hitsPointer = hits)
        fixed (PhysicsQueryResult* resultPointer = &result)
        {
            return ManagedRuntime.GetApi()->Physics->SpherecastAll(world.GetNative(), spherePointer, filter, hitsPointer, (uint)hits.Length, resultPointer) != 0;
        }
    }
}
