namespace SFG;

public static unsafe class Screen
{
    public static bool TryWorldToScreen(World world, in Vector3 worldPosition, out Vector2 screenPosition)
    {
        screenPosition = default;

        fixed (Vector3* worldPositionPointer = &worldPosition)
        fixed (Vector2* screenPositionPointer = &screenPosition)
        {
            return ManagedRuntime.GetApi()->Screen->WorldToScreen(world.GetNative(), worldPositionPointer, screenPositionPointer) != 0;
        }
    }

    public static bool TryScreenToWorld(World world, in Vector2 screenPosition, float normalizedDepth, out Vector3 worldPosition)
    {
        worldPosition = default;

        fixed (Vector2* screenPositionPointer = &screenPosition)
        fixed (Vector3* worldPositionPointer = &worldPosition)
        {
            return ManagedRuntime.GetApi()->Screen->ScreenToWorld(world.GetNative(), screenPositionPointer, normalizedDepth, worldPositionPointer) != 0;
        }
    }

}
