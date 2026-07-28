using System.Runtime.InteropServices;

namespace SFG;

[StructLayout(LayoutKind.Sequential)]
internal unsafe struct NativePlatformApi
{
    internal uint Size;
    internal uint Version;
    internal delegate* unmanaged[Cdecl]<byte, void> SetCursorVisible;
    internal delegate* unmanaged[Cdecl]<byte, void> LockCursor;
    internal delegate* unmanaged[Cdecl]<ushort, ushort, void> SetWindowSize;
    internal delegate* unmanaged[Cdecl]<WindowStyle, void> SetWindowStyle;
}
