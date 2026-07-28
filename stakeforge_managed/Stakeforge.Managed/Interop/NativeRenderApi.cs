using System.Runtime.InteropServices;

namespace SFG;

[StructLayout(LayoutKind.Sequential)]
internal unsafe struct NativeRenderApi
{
    internal uint Size;
    internal uint Version;
    internal delegate* unmanaged[Cdecl]<RenderResolution*, byte> GetRenderResolution;
    internal delegate* unmanaged[Cdecl]<ushort, ushort, byte> SetRenderResolution;
}
