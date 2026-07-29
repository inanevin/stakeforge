namespace SFG;

public static class Log
{
    public static void Info(string message)
    {
        ManagedRuntime.LogGameInfo(message);
    }

    public static void Error(string message)
    {
        ManagedRuntime.LogGameError(message);
    }

    public static void Warn(string message)
    {
        ManagedRuntime.LogGameWarn(message);
    }

    public static void Trace(string message)
    {
        ManagedRuntime.LogGameTrace(message);
    }
}
