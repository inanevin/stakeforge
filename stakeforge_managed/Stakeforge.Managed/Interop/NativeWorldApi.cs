using System.Runtime.InteropServices;

namespace SFG;

[StructLayout(LayoutKind.Sequential)]
internal unsafe struct NativeWorldApi
{
    internal uint Size;
    internal uint Version;
    internal delegate* unmanaged[Cdecl]<nint, byte*, uint> CreateEntity;
    internal delegate* unmanaged[Cdecl]<nint, uint, byte> DestroyEntity;
    internal delegate* unmanaged[Cdecl]<nint, uint, uint> DuplicateEntity;
    internal delegate* unmanaged[Cdecl]<nint, uint, uint, byte> AttachEntity;
    internal delegate* unmanaged[Cdecl]<nint, uint, byte> DetachEntity;
    internal delegate* unmanaged[Cdecl]<nint, uint, Vector3*, byte> SetEntityPosLocal;
    internal delegate* unmanaged[Cdecl]<nint, uint, Quaternion*, byte> SetEntityRotLocal;
    internal delegate* unmanaged[Cdecl]<nint, uint, Vector3*, byte> SetEntityScaleLocal;
    internal delegate* unmanaged[Cdecl]<nint, uint, Vector3*, Quaternion*, Vector3*, byte> TeleportEntity;
    internal delegate* unmanaged[Cdecl]<nint, uint, byte> MarkEntityTeleported;
    internal delegate* unmanaged[Cdecl]<nint, uint, Vector3*, byte> GetEntityPosLocal;
    internal delegate* unmanaged[Cdecl]<nint, uint, Quaternion*, byte> GetEntityRotLocal;
    internal delegate* unmanaged[Cdecl]<nint, uint, Vector3*, byte> GetEntityScaleLocal;
    internal delegate* unmanaged[Cdecl]<nint, uint, Vector3*, byte> GetEntityPosLastAbs;
    internal delegate* unmanaged[Cdecl]<nint, uint, Quaternion*, byte> GetEntityRotLastAbs;
    internal delegate* unmanaged[Cdecl]<nint, uint, Vector3*, byte> GetEntityScaleLastAbs;
    internal delegate* unmanaged[Cdecl]<nint, uint, Vector3*, Vector3*, byte> AbsPosToLocal;
    internal delegate* unmanaged[Cdecl]<nint, uint, Quaternion*, Quaternion*, byte> AbsRotToLocal;
    internal delegate* unmanaged[Cdecl]<nint, uint, Vector3*, Vector3*, byte> AbsScaleToLocal;
    internal delegate* unmanaged[Cdecl]<nint, byte*, uint> GetEntityWithName;
    internal delegate* unmanaged[Cdecl]<nint, byte*, Entity*, uint, uint> GetAllEntitiesWithName;
    internal delegate* unmanaged[Cdecl]<nint, ulong, uint> GetEntityWithComponent;
    internal delegate* unmanaged[Cdecl]<nint, ulong, Entity*, uint, uint> GetAllEntitiesWithComponent;
    internal delegate* unmanaged[Cdecl]<nint, uint, byte> IsAlive;
    internal delegate* unmanaged[Cdecl]<nint, uint, ulong, byte> HasComponent;
    internal delegate* unmanaged[Cdecl]<nint, uint, ulong, void*, uint, byte> GetComponent;
    internal delegate* unmanaged[Cdecl]<nint, uint, ulong, void*, uint, byte> AddComponent;
    internal delegate* unmanaged[Cdecl]<nint, uint, ulong, void*, uint, byte> SetComponent;
    internal delegate* unmanaged[Cdecl]<nint, uint, ulong, byte> RemoveComponent;
    internal delegate* unmanaged[Cdecl]<nint, ulong, uint> GetEntityWithTag;
    internal delegate* unmanaged[Cdecl]<nint, ulong, Entity*, uint, uint> GetAllEntitiesWithTag;
    internal delegate* unmanaged[Cdecl]<nint, uint, ulong, byte, byte> SetEntityTag;
    internal delegate* unmanaged[Cdecl]<nint, NativeWorldQueryComponent*, uint, NativeWorldQuery*, byte> QueryBegin;
    internal delegate* unmanaged[Cdecl]<NativeWorldQuery*, NativeWorldQueryRow*, byte> QueryNext;
    internal delegate* unmanaged[Cdecl]<NativeWorldQuery*, void> QueryEnd;
}
