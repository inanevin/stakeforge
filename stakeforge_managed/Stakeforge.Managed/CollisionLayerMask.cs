using System;
using System.Runtime.InteropServices;

namespace SFG;

[StructLayout(LayoutKind.Sequential)]
public readonly struct CollisionLayerMask : IEquatable<CollisionLayerMask>
{
    public static CollisionLayerMask None => new(0);
    public static CollisionLayerMask All => new(ulong.MaxValue);

    public readonly ulong Value;

    public CollisionLayerMask(ulong value)
    {
        Value = value;
    }

    public bool Contains(CollisionLayer layer)
    {
        return layer.Slot < 64 &&
            (Value & (1UL << layer.Slot)) != 0;
    }

    public bool Equals(CollisionLayerMask other)
    {
        return Value == other.Value;
    }

    public override bool Equals(object? obj)
    {
        return obj is CollisionLayerMask other && Equals(other);
    }

    public override int GetHashCode()
    {
        return Value.GetHashCode();
    }

    public static bool operator ==(
        CollisionLayerMask left,
        CollisionLayerMask right)
    {
        return left.Equals(right);
    }

    public static bool operator !=(
        CollisionLayerMask left,
        CollisionLayerMask right)
    {
        return !left.Equals(right);
    }

    public static implicit operator CollisionLayerMask(ulong value)
    {
        return new CollisionLayerMask(value);
    }

    public static implicit operator ulong(CollisionLayerMask mask)
    {
        return mask.Value;
    }
}
