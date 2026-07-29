using System;
using System.Runtime.InteropServices;

namespace SFG;

[Flags]
public enum PhysicsQueryFlags : byte
{
    Static = 1 << 0,
    Kinematic = 1 << 1,
    Dynamic = 1 << 2,
    Sensor = 1 << 3,
    Character = 1 << 4,
    All = Static | Kinematic | Dynamic | Sensor | Character,
}

[StructLayout(LayoutKind.Sequential)]
public struct PhysicsQueryFilter
{
    public static PhysicsQueryFilter Default => new()
    {
        CollisionLayers = ulong.MaxValue,
        IgnoredEntity = Entity.Invalid,
        Flags = PhysicsQueryFlags.All,
    };

    public ulong CollisionLayers;
    public ulong RequiredAnyTags;
    public ulong RequiredAllTags;
    public ulong ExcludedTags;
    public Entity IgnoredEntity;
    public PhysicsQueryFlags Flags;
}

[StructLayout(LayoutKind.Sequential)]
public struct PhysicsRaycast
{
    public Vector3 Origin;
    public Vector3 Direction;
    public float Distance;
}

[StructLayout(LayoutKind.Sequential)]
public struct PhysicsLinecast
{
    public Vector3 Start;
    public Vector3 End;
}

[StructLayout(LayoutKind.Sequential)]
public struct PhysicsSpherecast
{
    public Vector3 Origin;
    public Vector3 Direction;
    public float Radius;
    public float Distance;
}

[StructLayout(LayoutKind.Sequential)]
public struct PhysicsHit
{
    public Vector3 Position;
    public Vector3 Normal;
    public PhysicalMaterialHandle PhysicalMaterial;
    public Entity Entity;
    public float Distance;
    public float Fraction;
    public uint SubShapeId;
    private byte _isSensor;
    private byte _isCharacter;

    public bool IsSensor => _isSensor != 0;
    public bool IsCharacter => _isCharacter != 0;
}

[StructLayout(LayoutKind.Sequential)]
public struct PhysicsQueryResult
{
    public uint HitCount;
    private byte _overflow;

    public bool Overflow => _overflow != 0;
}

[StructLayout(LayoutKind.Sequential)]
public struct PhysicsBodyState
{
    public Vector3 Position;
    public Quaternion Rotation;
    public Vector3 LinearVelocity;
    public Vector3 AngularVelocity;
    private byte _isActive;

    public bool IsActive => _isActive != 0;
}

[StructLayout(LayoutKind.Sequential)]
public struct CharacterMoverState
{
    public Vector3 Velocity;
    public Vector3 GroundNormal;
    public Vector3 GroundVelocity;
    public Entity GroundEntity;
    public uint GroundSubShapeId;
    private byte _isGrounded;

    public bool IsGrounded => _isGrounded != 0;
}

[StructLayout(LayoutKind.Sequential)]
internal unsafe struct NativePhysicsApi
{
    internal uint Size;
    internal uint Version;
    internal delegate* unmanaged[Cdecl]<nint, uint, Vector3*, byte> SetBodyLinearVelocity;
    internal delegate* unmanaged[Cdecl]<nint, uint, Vector3*, byte> SetBodyAngularVelocity;
    internal delegate* unmanaged[Cdecl]<nint, uint, Vector3*, byte> AddBodyForce;
    internal delegate* unmanaged[Cdecl]<nint, uint, Vector3*, byte> AddBodyImpulse;
    internal delegate* unmanaged[Cdecl]<nint, uint, byte> WakeBody;
    internal delegate* unmanaged[Cdecl]<nint, uint, PhysicsBodyState*, byte> GetBodyState;
    internal delegate* unmanaged[Cdecl]<nint, PhysicsRaycast*, PhysicsQueryFilter*, byte> RaycastAny;
    internal delegate* unmanaged[Cdecl]<nint, PhysicsRaycast*, PhysicsQueryFilter*, PhysicsHit*, byte> RaycastClosest;
    internal delegate* unmanaged[Cdecl]<nint, PhysicsRaycast*, PhysicsQueryFilter*, PhysicsHit*, uint, PhysicsQueryResult*, byte> RaycastAll;
    internal delegate* unmanaged[Cdecl]<nint, PhysicsLinecast*, PhysicsQueryFilter*, PhysicsHit*, byte> LinecastClosest;
    internal delegate* unmanaged[Cdecl]<nint, PhysicsLinecast*, PhysicsHit*, uint, PhysicsQueryFilter*, void> LinecastClosestBatch;
    internal delegate* unmanaged[Cdecl]<nint, PhysicsSpherecast*, PhysicsQueryFilter*, byte> SpherecastAny;
    internal delegate* unmanaged[Cdecl]<nint, PhysicsSpherecast*, PhysicsQueryFilter*, PhysicsHit*, byte> SpherecastClosest;
    internal delegate* unmanaged[Cdecl]<nint, PhysicsSpherecast*, PhysicsQueryFilter*, PhysicsHit*, uint, PhysicsQueryResult*, byte> SpherecastAll;
    internal delegate* unmanaged[Cdecl]<nint, uint, Vector3*, byte> SetCharacterVelocity;
    internal delegate* unmanaged[Cdecl]<nint, uint, Vector3*, byte> AddCharacterVelocity;
    internal delegate* unmanaged[Cdecl]<nint, uint, float, byte> JumpCharacter;
    internal delegate* unmanaged[Cdecl]<nint, uint, Vector3*, byte> TeleportCharacter;
    internal delegate* unmanaged[Cdecl]<nint, uint, CharacterMoverState*, byte> GetCharacterState;
}
