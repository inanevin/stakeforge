using System;
using System.Runtime.InteropServices;

namespace SFG;

[StructLayout(LayoutKind.Sequential)]
public struct Vector3 : IEquatable<Vector3>
{
    public static Vector3 Zero => new(0.0f, 0.0f, 0.0f);
    public static Vector3 One => new(1.0f, 1.0f, 1.0f);
    public static Vector3 Up => new(0.0f, 1.0f, 0.0f);
    public static Vector3 Down => new(0.0f, -1.0f, 0.0f);
    public static Vector3 Forward => new(0.0f, 0.0f, -1.0f);
    public static Vector3 Back => new(0.0f, 0.0f, 1.0f);
    public static Vector3 Right => new(1.0f, 0.0f, 0.0f);
    public static Vector3 Left => new(-1.0f, 0.0f, 0.0f);

    public float X;
    public float Y;
    public float Z;

    public Vector3(float x, float y, float z)
    {
        X = x;
        Y = y;
        Z = z;
    }

    public readonly float Length => Math.Sqrt(LengthSquared);
    public readonly float LengthSquared => X * X + Y * Y + Z * Z;
    public readonly float Magnitude => Length;
    public readonly float MagnitudeSquared => LengthSquared;

    public readonly Vector3 Normalized
    {
        get
        {
            float length = Length;
            return length > Math.Epsilon ? this / length : Zero;
        }
    }

    public void Normalize()
    {
        this = Normalized;
    }

    public static Vector3 Clamp(Vector3 value, Vector3 minimum, Vector3 maximum)
    {
        return new Vector3(Math.Clamp(value.X, minimum.X, maximum.X), Math.Clamp(value.Y, minimum.Y, maximum.Y), Math.Clamp(value.Z, minimum.Z, maximum.Z));
    }

    public static Vector3 ClampMagnitude(Vector3 value, float maximumLength)
    {
        float length = value.Length;
        return length > maximumLength ? value / length * maximumLength : value;
    }

    public static Vector3 Cross(Vector3 a, Vector3 b)
    {
        return new Vector3(a.Y * b.Z - a.Z * b.Y, a.Z * b.X - a.X * b.Z, a.X * b.Y - a.Y * b.X);
    }

    public static Vector3 Abs(Vector3 value) => new(Math.Abs(value.X), Math.Abs(value.Y), Math.Abs(value.Z));
    public static Vector3 Min(Vector3 a, Vector3 b) => new(Math.Min(a.X, b.X), Math.Min(a.Y, b.Y), Math.Min(a.Z, b.Z));
    public static Vector3 Max(Vector3 a, Vector3 b) => new(Math.Max(a.X, b.X), Math.Max(a.Y, b.Y), Math.Max(a.Z, b.Z));
    public static Vector3 Lerp(Vector3 a, Vector3 b, float amount) => new(Math.Lerp(a.X, b.X, amount), Math.Lerp(a.Y, b.Y, amount), Math.Lerp(a.Z, b.Z, amount));
    public static float Dot(Vector3 a, Vector3 b) => a.X * b.X + a.Y * b.Y + a.Z * b.Z;
    public static float Distance(Vector3 a, Vector3 b) => (a - b).Length;
    public static float DistanceSquared(Vector3 a, Vector3 b) => (a - b).LengthSquared;

    public static Vector3 Project(Vector3 value, Vector3 onNormal)
    {
        float denominator = onNormal.LengthSquared;
        return denominator < Math.Epsilon ? Zero : onNormal * (Dot(value, onNormal) / denominator);
    }

    public static Vector3 Reflect(Vector3 value, Vector3 normal)
    {
        Vector3 unitNormal = normal.Normalized;
        return unitNormal.IsZero() ? -value : value - unitNormal * (2.0f * Dot(value, unitNormal));
    }

    public static Vector3 Rotate(Vector3 value, Vector3 axis, float angleDegrees)
    {
        Vector3 unitAxis = axis.Normalized;

        if (unitAxis.IsZero())
        {
            return value;
        }

        float radians = Math.DegreesToRadians(angleDegrees);
        float cosine = Math.Cos(radians);
        float sine = Math.Sin(radians);
        return value * cosine + Cross(unitAxis, value) * sine + unitAxis * (Dot(unitAxis, value) * (1.0f - cosine));
    }

    public readonly bool ApproximatelyEquals(Vector3 other, float epsilon = Math.Epsilon)
    {
        return Math.ApproximatelyEqual(X, other.X, epsilon) && Math.ApproximatelyEqual(Y, other.Y, epsilon) && Math.ApproximatelyEqual(Z, other.Z, epsilon);
    }

    public readonly bool IsZero(float epsilon = Math.Epsilon)
    {
        return Math.Abs(X) < epsilon && Math.Abs(Y) < epsilon && Math.Abs(Z) < epsilon;
    }

    public readonly bool Equals(Vector3 other) => X.Equals(other.X) && Y.Equals(other.Y) && Z.Equals(other.Z);
    public override readonly bool Equals(object? obj) => obj is Vector3 other && Equals(other);
    public override readonly int GetHashCode() => HashCode.Combine(X, Y, Z);
    public override readonly string ToString() => $"({X}, {Y}, {Z})";

    public static Vector3 operator +(Vector3 a, Vector3 b) => new(a.X + b.X, a.Y + b.Y, a.Z + b.Z);
    public static Vector3 operator -(Vector3 a, Vector3 b) => new(a.X - b.X, a.Y - b.Y, a.Z - b.Z);
    public static Vector3 operator -(Vector3 value) => new(-value.X, -value.Y, -value.Z);
    public static Vector3 operator *(Vector3 value, float scalar) => new(value.X * scalar, value.Y * scalar, value.Z * scalar);
    public static Vector3 operator *(float scalar, Vector3 value) => value * scalar;
    public static Vector3 operator *(Vector3 a, Vector3 b) => new(a.X * b.X, a.Y * b.Y, a.Z * b.Z);
    public static Vector3 operator /(Vector3 value, float scalar) => scalar == 0.0f ? Zero : new(value.X / scalar, value.Y / scalar, value.Z / scalar);
    public static bool operator ==(Vector3 a, Vector3 b) => a.Equals(b);
    public static bool operator !=(Vector3 a, Vector3 b) => !a.Equals(b);
}
