using System;
using System.Runtime.InteropServices;

namespace SFG;

[StructLayout(LayoutKind.Sequential)]
public readonly struct Entity : IEquatable<Entity>
{
    public static Entity Invalid { get; } = new(uint.MaxValue);

    public readonly uint Id;

    internal Entity(uint id)
    {
        Id = id;
    }

    public bool IsValid => Id != uint.MaxValue;

    public bool Equals(Entity other)
    {
        return Id == other.Id;
    }

    public override bool Equals(object? obj)
    {
        return obj is Entity other && Equals(other);
    }

    public override int GetHashCode()
    {
        return Id.GetHashCode();
    }

    public static bool operator ==(Entity left, Entity right)
    {
        return left.Equals(right);
    }

    public static bool operator !=(Entity left, Entity right)
    {
        return !left.Equals(right);
    }
}
