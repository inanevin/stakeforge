namespace SFG;

public static unsafe class Stats
{
    public static float GetMainThreadTimeMilliseconds()
    {
        return ManagedRuntime.GetApi()->Stats->GetMainThreadTimeMilliseconds();
    }

    public static float GetMainThreadFps()
    {
        return ManagedRuntime.GetApi()->Stats->GetMainThreadFps();
    }

    public static float GetRenderWorkTimeMilliseconds()
    {
        return ManagedRuntime.GetApi()->Stats->GetRenderWorkTimeMilliseconds();
    }

    public static float GetRenderThreadTimeMilliseconds()
    {
        return ManagedRuntime.GetApi()->Stats->GetRenderThreadTimeMilliseconds();
    }

    public static float GetRenderThreadFps()
    {
        return ManagedRuntime.GetApi()->Stats->GetRenderThreadFps();
    }
}
