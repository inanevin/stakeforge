using System;
using System.Text;

namespace SFG;

public static class Hash
{
    public const ulong FnvOffset = 14695981039346656037UL;
    public const ulong FnvPrime = 1099511628211UL;
    public const ulong StringIdOffset = 1469598103934665603UL;

    public static ulong StringId(string value)
    {
        ArgumentNullException.ThrowIfNull(value);

        int byteCount = Encoding.UTF8.GetByteCount(value);
        Span<byte> bytes = byteCount <= 256 ? stackalloc byte[byteCount] : new byte[byteCount];
        Encoding.UTF8.GetBytes(value, bytes);
        return StringId(bytes);
    }

    public static ulong StringId(ReadOnlySpan<byte> utf8Bytes)
    {
        return Fnv1A64(StringIdOffset, utf8Bytes);
    }

    public static ulong Fnv1A64(string value)
    {
        ArgumentNullException.ThrowIfNull(value);

        int byteCount = Encoding.UTF8.GetByteCount(value);
        Span<byte> bytes = byteCount <= 256 ? stackalloc byte[byteCount] : new byte[byteCount];
        Encoding.UTF8.GetBytes(value, bytes);
        return Fnv1A64(bytes);
    }

    public static ulong Fnv1A64(ReadOnlySpan<byte> bytes)
    {
        return Fnv1A64(FnvOffset, bytes);
    }

    public static ulong Fnv1A64(ulong seed, ReadOnlySpan<byte> bytes)
    {
        ulong hash = seed;

        unchecked
        {
            foreach (byte value in bytes)
            {
                hash = (hash ^ value) * FnvPrime;
            }
        }

        return hash;
    }
}
