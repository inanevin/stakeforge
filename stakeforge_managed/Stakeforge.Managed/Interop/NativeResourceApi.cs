using System.Runtime.InteropServices;

namespace SFG;

[StructLayout(LayoutKind.Sequential)]
internal unsafe struct NativeResourceApi
{
    internal uint Size;
    internal uint Version;
    internal delegate* unmanaged[Cdecl]<ulong, ulong, float, byte> UpdateMaterialParameterF32;
    internal delegate* unmanaged[Cdecl]<ulong, ulong, Vector2*, byte> UpdateMaterialParameterVec2;
    internal delegate* unmanaged[Cdecl]<ulong, ulong, Vector4*, byte> UpdateMaterialParameterVec4;
    internal delegate* unmanaged[Cdecl]<ulong, ulong, uint, byte> UpdateMaterialParameterU32;
    internal delegate* unmanaged[Cdecl]<ulong, ulong, ulong, byte> UpdateMaterialTexture;
    internal delegate* unmanaged[Cdecl]<ulong, ulong, ulong, byte> UpdateMaterialSampler;
}
