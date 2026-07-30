using System.Runtime.InteropServices;

namespace SFG;

[StructLayout(LayoutKind.Sequential)]
internal unsafe struct NativeStatsApi
{
    internal uint Size;
    internal uint Version;
    internal delegate* unmanaged[Cdecl]<float> GetMainThreadTimeMilliseconds;
    internal delegate* unmanaged[Cdecl]<float> GetMainThreadFps;
    internal delegate* unmanaged[Cdecl]<float> GetRenderWorkTimeMilliseconds;
    internal delegate* unmanaged[Cdecl]<float> GetRenderThreadTimeMilliseconds;
    internal delegate* unmanaged[Cdecl]<float> GetRenderThreadFps;
}
