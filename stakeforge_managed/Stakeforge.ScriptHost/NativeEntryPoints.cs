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
            ProjectAssemblyRuntime.Unload();
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

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    public static int LoadProjectAssembly(byte* assemblyPath)
    {
        try
        {
            string? path = Marshal.PtrToStringUTF8((nint)assemblyPath);

            if (string.IsNullOrWhiteSpace(path))
            {
                ManagedRuntime.TryLogError("the C# project assembly path is empty.");
                return -1;
            }

            ProjectAssemblyRuntime.Load(path);
            return 0;
        }
        catch (Exception exception)
        {
            ManagedRuntime.TryLogError(exception);
            return -2;
        }
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    public static int UnloadProjectAssembly()
    {
        try
        {
            ProjectAssemblyRuntime.Unload();
            return 0;
        }
        catch (Exception exception)
        {
            ManagedRuntime.TryLogError(exception);
            return -2;
        }
    }
}
