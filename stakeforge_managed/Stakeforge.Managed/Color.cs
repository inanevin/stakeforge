using System;
using System.Runtime.InteropServices;

namespace SFG;

[StructLayout(LayoutKind.Sequential)]
public struct Color : IEquatable<Color>
{
    public static Color Clear => new(0.0f, 0.0f, 0.0f, 0.0f);
    public static Color Black => new(0.0f, 0.0f, 0.0f, 1.0f);
    public static Color White => new(1.0f, 1.0f, 1.0f, 1.0f);
    public static Color Red => new(1.0f, 0.0f, 0.0f, 1.0f);
    public static Color Green => new(0.0f, 1.0f, 0.0f, 1.0f);
    public static Color Blue => new(0.0f, 0.0f, 1.0f, 1.0f);
    public static Color Yellow => new(1.0f, 1.0f, 0.0f, 1.0f);
    public static Color Cyan => new(0.0f, 1.0f, 1.0f, 1.0f);
    public static Color Magenta => new(1.0f, 0.0f, 1.0f, 1.0f);

    public float R;
    public float G;
    public float B;
    public float A;

    public Color(float red, float green, float blue, float alpha = 1.0f)
    {
        R = red;
        G = green;
        B = blue;
        A = alpha;
    }

    public readonly bool Equals(Color other)
    {
        return R == other.R && G == other.G && B == other.B && A == other.A;
    }

    public override readonly bool Equals(object? obj)
    {
        return obj is Color other && Equals(other);
    }

    public override readonly int GetHashCode()
    {
        return HashCode.Combine(R, G, B, A);
    }

    public static bool operator ==(Color left, Color right)
    {
        return left.Equals(right);
    }

    public static bool operator !=(Color left, Color right)
    {
        return !left.Equals(right);
    }
}
