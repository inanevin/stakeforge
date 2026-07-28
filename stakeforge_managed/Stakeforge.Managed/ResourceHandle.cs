using System;
using System.Runtime.InteropServices;

namespace SFG;

[StructLayout(LayoutKind.Sequential)]
public readonly struct ResourceHandle : IEquatable<ResourceHandle>
{
    public static ResourceHandle Invalid { get; } = new(ulong.MaxValue);

    public readonly ulong Id;

    public ResourceHandle(ulong id)
    {
        Id = id;
    }

    public bool IsValid => Id != ulong.MaxValue;

    public bool Equals(ResourceHandle other)
    {
        return Id == other.Id;
    }

    public override bool Equals(object? obj)
    {
        return obj is ResourceHandle other && Equals(other);
    }

    public override int GetHashCode()
    {
        return Id.GetHashCode();
    }

    public static bool operator ==(ResourceHandle left, ResourceHandle right)
    {
        return left.Equals(right);
    }

    public static bool operator !=(ResourceHandle left, ResourceHandle right)
    {
        return !left.Equals(right);
    }
}
