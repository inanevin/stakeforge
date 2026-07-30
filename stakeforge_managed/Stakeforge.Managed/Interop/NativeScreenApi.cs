using System.Runtime.InteropServices;

namespace SFG;

[StructLayout(LayoutKind.Sequential)]
internal unsafe struct NativeScreenApi
{
    internal uint Size;
    internal uint Version;
    internal delegate* unmanaged[Cdecl]<nint, Vector3*, Vector2*, byte> WorldToScreen;
    internal delegate* unmanaged[Cdecl]<nint, Vector2*, float, Vector3*, byte> ScreenToWorld;
}
