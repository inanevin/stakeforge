using System;
using System.Runtime.InteropServices;

namespace SFG;

[StructLayout(LayoutKind.Sequential)]
public readonly struct CollisionLayer : IEquatable<CollisionLayer>
{
    public readonly byte Slot;

    public CollisionLayer(byte slot)
    {
        Slot = slot;
    }

    public bool Equals(CollisionLayer other)
    {
        return Slot == other.Slot;
    }

    public override bool Equals(object? obj)
    {
        return obj is CollisionLayer other && Equals(other);
    }

    public override int GetHashCode()
    {
        return Slot.GetHashCode();
    }

    public static bool operator ==(CollisionLayer left, CollisionLayer right)
    {
        return left.Equals(right);
    }

    public static bool operator !=(CollisionLayer left, CollisionLayer right)
    {
        return !left.Equals(right);
    }

    public static implicit operator CollisionLayer(byte slot)
    {
        return new CollisionLayer(slot);
    }

    public static implicit operator byte(CollisionLayer layer)
    {
        return layer.Slot;
    }
}
