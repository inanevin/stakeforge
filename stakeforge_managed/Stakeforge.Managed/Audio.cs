namespace SFG;

public static unsafe class Audio
{
    public static bool Play(World world, Entity entity)
    {
        return ManagedRuntime.GetApi()->Audio->Play(world.GetNative(), entity.Id) != 0;
    }

    public static bool Pause(World world, Entity entity)
    {
        return ManagedRuntime.GetApi()->Audio->Pause(world.GetNative(), entity.Id) != 0;
    }

    public static bool Stop(World world, Entity entity)
    {
        return ManagedRuntime.GetApi()->Audio->Stop(world.GetNative(), entity.Id) != 0;
    }

    public static void PauseAll(World world)
    {
        ManagedRuntime.GetApi()->Audio->PauseAll(world.GetNative());
    }

    public static void ResumeAll(World world)
    {
        ManagedRuntime.GetApi()->Audio->ResumeAll(world.GetNative());
    }
}
