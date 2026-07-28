using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Stakeforge.ScriptHost;

public static unsafe class NativeEntryPoints
{
    private const uint ApiVersion = 1;

    private static NativeApi* _api;

    [StructLayout(LayoutKind.Sequential)]
    public struct NativeApi
    {
        public uint Size;
        public uint Version;
        public delegate* unmanaged[Cdecl]<byte*, void> LogInfo;
        public delegate* unmanaged[Cdecl]<byte*, void> LogError;
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    public static int Initialize(NativeApi* api)
    {
        try
        {
            if (api == null ||
                api->Size < sizeof(NativeApi) ||
                api->Version != ApiVersion ||
                api->LogInfo == null ||
                api->LogError == null)
            {
                return -1;
            }

            _api = api;
            Log(_api->LogInfo, "managed scripting host initialized correctly.");
            return 0;
        }
        catch (Exception exception)
        {
            TryReportException(api, exception);
            _api = null;
            return -2;
        }
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    public static int RunPhaseOneTest()
    {
        try
        {
            try
            {
                throw new InvalidOperationException("phase one exception boundary test");
            }
            catch (InvalidOperationException)
            {
            }

            Log(_api->LogInfo, "managed scripting phase one test passed.");
            return 0;
        }
        catch (Exception exception)
        {
            TryReportException(_api, exception);
            return -2;
        }
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    public static int Shutdown()
    {
        try
        {
            Log(_api->LogInfo, "managed scripting host shut down correctly.");
            _api = null;
            return 0;
        }
        catch (Exception exception)
        {
            TryReportException(_api, exception);
            _api = null;
            return -2;
        }
    }

    private static void TryReportException(NativeApi* api, Exception exception)
    {
        try
        {
            if (api != null && api->LogError != null)
            {
                Log(api->LogError, exception.ToString());
            }
        }
        catch
        {
        }
    }

    private static void Log(delegate* unmanaged[Cdecl]<byte*, void> callback, string message)
    {
        nint utf8Message = Marshal.StringToCoTaskMemUTF8(message);

        try
        {
            callback((byte*)utf8Message);
        }
        finally
        {
            Marshal.FreeCoTaskMem(utf8Message);
        }
    }
}
