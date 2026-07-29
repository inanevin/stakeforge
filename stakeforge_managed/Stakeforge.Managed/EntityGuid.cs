using System;
using System.Runtime.InteropServices;

namespace SFG;

[StructLayout(LayoutKind.Sequential)]
public readonly struct EntityGuid : IEquatable<EntityGuid>
{
    public static EntityGuid Invalid { get; } = new(ulong.MaxValue);

    public readonly ulong Id;

    public EntityGuid(ulong id)
    {
        Id = id;
    }

    public bool IsValid => Id != ulong.MaxValue;

    public bool Equals(EntityGuid other)
    {
        return Id == other.Id;
    }

    public override bool Equals(object? obj)
    {
        return obj is EntityGuid other && Equals(other);
    }

    public override int GetHashCode()
    {
        return Id.GetHashCode();
    }

    public static bool operator ==(EntityGuid left, EntityGuid right)
    {
        return left.Equals(right);
    }

    public static bool operator !=(EntityGuid left, EntityGuid right)
    {
        return !left.Equals(right);
    }
}
