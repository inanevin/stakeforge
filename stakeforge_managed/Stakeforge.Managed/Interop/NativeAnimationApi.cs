using System.Runtime.InteropServices;

namespace SFG;

[StructLayout(LayoutKind.Sequential)]
internal unsafe struct NativeAnimationApi
{
    internal uint Size;
    internal uint Version;
    internal delegate* unmanaged[Cdecl]<nint, uint, ulong, Vector3*, byte> GetSlotPosAbs;
    internal delegate* unmanaged[Cdecl]<nint, uint, ulong, Quaternion*, byte> GetSlotRotAbs;
    internal delegate* unmanaged[Cdecl]<nint, uint, ulong, float, byte> SetGraphParameterF32;
    internal delegate* unmanaged[Cdecl]<nint, uint, ulong, Vector2*, byte> SetGraphParameterVec2;
    internal delegate* unmanaged[Cdecl]<nint, uint, ulong, Vector3*, byte> SetGraphParameterVec3;
    internal delegate* unmanaged[Cdecl]<nint, uint, ulong, Quaternion*, byte> SetGraphParameterQuat;
    internal delegate* unmanaged[Cdecl]<nint, uint, ulong, byte, byte> SetGraphParameterBool;
    internal delegate* unmanaged[Cdecl]<nint, uint, ulong, float*, byte> GetGraphParameterF32;
    internal delegate* unmanaged[Cdecl]<nint, uint, ulong, Vector2*, byte> GetGraphParameterVec2;
    internal delegate* unmanaged[Cdecl]<nint, uint, ulong, Vector3*, byte> GetGraphParameterVec3;
    internal delegate* unmanaged[Cdecl]<nint, uint, ulong, Quaternion*, byte> GetGraphParameterQuat;
    internal delegate* unmanaged[Cdecl]<nint, uint, ulong, byte*, byte> GetGraphParameterBool;
}
