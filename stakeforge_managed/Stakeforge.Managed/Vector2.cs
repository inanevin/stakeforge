using System;
using System.Runtime.InteropServices;

namespace SFG;

[StructLayout(LayoutKind.Sequential)]
public struct Vector2 : IEquatable<Vector2>
{
    public static Vector2 Zero => new(0.0f, 0.0f);
    public static Vector2 One => new(1.0f, 1.0f);

    public float X;
    public float Y;

    public Vector2(float x, float y)
    {
        X = x;
        Y = y;
    }

    public readonly float Length => Math.Sqrt(LengthSquared);
    public readonly float LengthSquared => X * X + Y * Y;
    public readonly float Magnitude => Length;
    public readonly float MagnitudeSquared => LengthSquared;

    public readonly Vector2 Normalized
    {
        get
        {
            float length = Length;
            return length > Math.Epsilon ? this / length : Zero;
        }
    }

    public static Vector2 Clamp(Vector2 value, Vector2 minimum, Vector2 maximum)
    {
        return new Vector2(Math.Clamp(value.X, minimum.X, maximum.X), Math.Clamp(value.Y, minimum.Y, maximum.Y));
    }

    public static Vector2 ClampMagnitude(Vector2 value, float maximumLength)
    {
        float length = value.Length;
        return length > maximumLength ? value / length * maximumLength : value;
    }

    public static Vector2 Abs(Vector2 value) => new(Math.Abs(value.X), Math.Abs(value.Y));
    public static Vector2 Min(Vector2 a, Vector2 b) => new(Math.Min(a.X, b.X), Math.Min(a.Y, b.Y));
    public static Vector2 Max(Vector2 a, Vector2 b) => new(Math.Max(a.X, b.X), Math.Max(a.Y, b.Y));
    public static Vector2 Lerp(Vector2 a, Vector2 b, float amount) => new(Math.Lerp(a.X, b.X, amount), Math.Lerp(a.Y, b.Y, amount));
    public static float Dot(Vector2 a, Vector2 b) => a.X * b.X + a.Y * b.Y;
    public static float Distance(Vector2 a, Vector2 b) => (a - b).Length;
    public static float DistanceSquared(Vector2 a, Vector2 b) => (a - b).LengthSquared;

    public static float DistanceSquaredToSegment(Vector2 point, Vector2 a, Vector2 b)
    {
        Vector2 segment = b - a;
        float segmentLengthSquared = segment.LengthSquared;
        float amount = segmentLengthSquared > Math.Epsilon ? Math.Clamp(Dot(point - a, segment) / segmentLengthSquared, 0.0f, 1.0f) : 0.0f;
        return (point - (a + segment * amount)).LengthSquared;
    }

    public static float Angle(Vector2 a, Vector2 b)
    {
        float lengths = a.Length * b.Length;

        if (lengths == 0.0f)
        {
            return 0.0f;
        }

        return Math.RadiansToDegrees(Math.Acos(Math.Clamp(Dot(a, b) / lengths, -1.0f, 1.0f)));
    }

    public readonly bool ApproximatelyEquals(Vector2 other, float epsilon = Math.Epsilon)
    {
        return Math.ApproximatelyEqual(X, other.X, epsilon) && Math.ApproximatelyEqual(Y, other.Y, epsilon);
    }

    public readonly bool IsZero(float epsilon = Math.Epsilon)
    {
        return Math.Abs(X) < epsilon && Math.Abs(Y) < epsilon;
    }

    public readonly bool Equals(Vector2 other) => X.Equals(other.X) && Y.Equals(other.Y);
    public override readonly bool Equals(object? obj) => obj is Vector2 other && Equals(other);
    public override readonly int GetHashCode() => HashCode.Combine(X, Y);
    public override readonly string ToString() => $"({X}, {Y})";

    public static Vector2 operator +(Vector2 a, Vector2 b) => new(a.X + b.X, a.Y + b.Y);
    public static Vector2 operator -(Vector2 a, Vector2 b) => new(a.X - b.X, a.Y - b.Y);
    public static Vector2 operator -(Vector2 value) => new(-value.X, -value.Y);
    public static Vector2 operator *(Vector2 value, float scalar) => new(value.X * scalar, value.Y * scalar);
    public static Vector2 operator *(float scalar, Vector2 value) => value * scalar;
    public static Vector2 operator *(Vector2 a, Vector2 b) => new(a.X * b.X, a.Y * b.Y);
    public static Vector2 operator /(Vector2 value, float scalar) => scalar == 0.0f ? Zero : new(value.X / scalar, value.Y / scalar);
    public static bool operator ==(Vector2 a, Vector2 b) => a.Equals(b);
    public static bool operator !=(Vector2 a, Vector2 b) => !a.Equals(b);
}
