using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using SFG;

namespace SFG.ScriptHost;

public static unsafe class NativeEntryPoints
{
    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    public static int Initialize(void* api)
    {
        try
        {
            int result = ManagedRuntime.Initialize(api);

            if (result == 0)
            {
                ManagedRuntime.LogInfo("managed scripting host initialized correctly.");
            }

            return result;
        }
        catch (Exception exception)
        {
            ManagedRuntime.TryLogError(api, exception);
            ManagedRuntime.Shutdown();
            return -2;
        }
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    public static int Shutdown()
    {
        try
        {
            ManagedRuntime.LogInfo("managed scripting host shut down correctly.");
            return 0;
        }
        catch (Exception exception)
        {
            ManagedRuntime.TryLogError(exception);
            return -2;
        }
        finally
        {
            ManagedRuntime.Shutdown();
        }
    }
}
