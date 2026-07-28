namespace SFG;

public static unsafe class Animation
{
    public static bool TryGetSlotPositionAbsolute(World world, Entity entity, ulong slotNameHash, out Vector3 position)
    {
        position = default;

        fixed (Vector3* positionPointer = &position)
        {
            return ManagedRuntime.GetApi()->Animation->GetSlotPosAbs(world.GetNative(), entity.Id, slotNameHash, positionPointer) != 0;
        }
    }

    public static bool TryGetSlotRotationAbsolute(World world, Entity entity, ulong slotNameHash, out Quaternion rotation)
    {
        rotation = default;

        fixed (Quaternion* rotationPointer = &rotation)
        {
            return ManagedRuntime.GetApi()->Animation->GetSlotRotAbs(world.GetNative(), entity.Id, slotNameHash, rotationPointer) != 0;
        }
    }

    public static bool SetGraphParameter(World world, Entity entity, ulong parameterHash, float value)
    {
        return ManagedRuntime.GetApi()->Animation->SetGraphParameterF32(world.GetNative(), entity.Id, parameterHash, value) != 0;
    }

    public static bool SetGraphParameter(World world, Entity entity, ulong parameterHash, Vector2 value)
    {
        return ManagedRuntime.GetApi()->Animation->SetGraphParameterVec2(world.GetNative(), entity.Id, parameterHash, &value) != 0;
    }

    public static bool SetGraphParameter(World world, Entity entity, ulong parameterHash, Vector3 value)
    {
        return ManagedRuntime.GetApi()->Animation->SetGraphParameterVec3(world.GetNative(), entity.Id, parameterHash, &value) != 0;
    }

    public static bool SetGraphParameter(World world, Entity entity, ulong parameterHash, Quaternion value)
    {
        return ManagedRuntime.GetApi()->Animation->SetGraphParameterQuat(world.GetNative(), entity.Id, parameterHash, &value) != 0;
    }

    public static bool SetGraphParameter(World world, Entity entity, ulong parameterHash, bool value)
    {
        return ManagedRuntime.GetApi()->Animation->SetGraphParameterBool(world.GetNative(), entity.Id, parameterHash, value ? (byte)1 : (byte)0) != 0;
    }

    public static bool TryGetGraphParameter(World world, Entity entity, ulong parameterHash, out float value)
    {
        value = default;

        fixed (float* valuePointer = &value)
        {
            return ManagedRuntime.GetApi()->Animation->GetGraphParameterF32(world.GetNative(), entity.Id, parameterHash, valuePointer) != 0;
        }
    }

    public static bool TryGetGraphParameter(World world, Entity entity, ulong parameterHash, out Vector2 value)
    {
        value = default;

        fixed (Vector2* valuePointer = &value)
        {
            return ManagedRuntime.GetApi()->Animation->GetGraphParameterVec2(world.GetNative(), entity.Id, parameterHash, valuePointer) != 0;
        }
    }

    public static bool TryGetGraphParameter(World world, Entity entity, ulong parameterHash, out Vector3 value)
    {
        value = default;

        fixed (Vector3* valuePointer = &value)
        {
            return ManagedRuntime.GetApi()->Animation->GetGraphParameterVec3(world.GetNative(), entity.Id, parameterHash, valuePointer) != 0;
        }
    }

    public static bool TryGetGraphParameter(World world, Entity entity, ulong parameterHash, out Quaternion value)
    {
        value = default;

        fixed (Quaternion* valuePointer = &value)
        {
            return ManagedRuntime.GetApi()->Animation->GetGraphParameterQuat(world.GetNative(), entity.Id, parameterHash, valuePointer) != 0;
        }
    }

    public static bool TryGetGraphParameter(World world, Entity entity, ulong parameterHash, out bool value)
    {
        byte nativeValue = 0;
        bool result = ManagedRuntime.GetApi()->Animation->GetGraphParameterBool(world.GetNative(), entity.Id, parameterHash, &nativeValue) != 0;
        value = nativeValue != 0;
        return result;
    }
}
