using System;
using System.Runtime.InteropServices;

namespace SFG;

[StructLayout(LayoutKind.Sequential)]
public struct Vector4 : IEquatable<Vector4>
{
    public static Vector4 Zero => new(0.0f, 0.0f, 0.0f, 0.0f);
    public static Vector4 One => new(1.0f, 1.0f, 1.0f, 1.0f);

    public float X;
    public float Y;
    public float Z;
    public float W;

    public Vector4(float x, float y, float z, float w)
    {
        X = x;
        Y = y;
        Z = z;
        W = w;
    }

    public readonly float Length => Math.Sqrt(LengthSquared);
    public readonly float LengthSquared => X * X + Y * Y + Z * Z + W * W;
    public readonly float Magnitude => Length;
    public readonly float MagnitudeSquared => LengthSquared;

    public readonly Vector4 Normalized
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

    public static Vector4 Clamp(Vector4 value, Vector4 minimum, Vector4 maximum)
    {
        return new Vector4(Math.Clamp(value.X, minimum.X, maximum.X), Math.Clamp(value.Y, minimum.Y, maximum.Y), Math.Clamp(value.Z, minimum.Z, maximum.Z), Math.Clamp(value.W, minimum.W, maximum.W));
    }

    public static Vector4 Abs(Vector4 value) => new(Math.Abs(value.X), Math.Abs(value.Y), Math.Abs(value.Z), Math.Abs(value.W));
    public static Vector4 Min(Vector4 a, Vector4 b) => new(Math.Min(a.X, b.X), Math.Min(a.Y, b.Y), Math.Min(a.Z, b.Z), Math.Min(a.W, b.W));
    public static Vector4 Max(Vector4 a, Vector4 b) => new(Math.Max(a.X, b.X), Math.Max(a.Y, b.Y), Math.Max(a.Z, b.Z), Math.Max(a.W, b.W));
    public static Vector4 Lerp(Vector4 a, Vector4 b, float amount) => new(Math.Lerp(a.X, b.X, amount), Math.Lerp(a.Y, b.Y, amount), Math.Lerp(a.Z, b.Z, amount), Math.Lerp(a.W, b.W, amount));
    public static float Dot(Vector4 a, Vector4 b) => a.X * b.X + a.Y * b.Y + a.Z * b.Z + a.W * b.W;
    public static float Distance(Vector4 a, Vector4 b) => (a - b).Length;
    public static float DistanceSquared(Vector4 a, Vector4 b) => (a - b).LengthSquared;

    public static Vector4 Project(Vector4 value, Vector4 onNormal)
    {
        float denominator = onNormal.LengthSquared;
        return denominator < Math.Epsilon ? Zero : onNormal * (Dot(value, onNormal) / denominator);
    }

    public static Vector4 Rotate(Vector4 value, Vector4 axis, float angleDegrees)
    {
        Vector3 rotated = Vector3.Rotate(new Vector3(value.X, value.Y, value.Z), new Vector3(axis.X, axis.Y, axis.Z), angleDegrees);
        return new Vector4(rotated.X, rotated.Y, rotated.Z, value.W);
    }

    public readonly bool IsPointInside(float x, float y) => x >= X && x <= X + Z && y >= Y && y <= Y + W;

    public readonly bool ApproximatelyEquals(Vector4 other, float epsilon = Math.Epsilon)
    {
        return Math.ApproximatelyEqual(X, other.X, epsilon) && Math.ApproximatelyEqual(Y, other.Y, epsilon) && Math.ApproximatelyEqual(Z, other.Z, epsilon) && Math.ApproximatelyEqual(W, other.W, epsilon);
    }

    public readonly bool IsZero(float epsilon = Math.Epsilon)
    {
        return Math.Abs(X) < epsilon && Math.Abs(Y) < epsilon && Math.Abs(Z) < epsilon && Math.Abs(W) < epsilon;
    }

    public readonly bool Equals(Vector4 other) => X.Equals(other.X) && Y.Equals(other.Y) && Z.Equals(other.Z) && W.Equals(other.W);
    public override readonly bool Equals(object? obj) => obj is Vector4 other && Equals(other);
    public override readonly int GetHashCode() => HashCode.Combine(X, Y, Z, W);
    public override readonly string ToString() => $"({X}, {Y}, {Z}, {W})";

    public static Vector4 operator +(Vector4 a, Vector4 b) => new(a.X + b.X, a.Y + b.Y, a.Z + b.Z, a.W + b.W);
    public static Vector4 operator -(Vector4 a, Vector4 b) => new(a.X - b.X, a.Y - b.Y, a.Z - b.Z, a.W - b.W);
    public static Vector4 operator -(Vector4 value) => new(-value.X, -value.Y, -value.Z, -value.W);
    public static Vector4 operator *(Vector4 value, float scalar) => new(value.X * scalar, value.Y * scalar, value.Z * scalar, value.W * scalar);
    public static Vector4 operator *(float scalar, Vector4 value) => value * scalar;
    public static Vector4 operator *(Vector4 a, Vector4 b) => new(a.X * b.X, a.Y * b.Y, a.Z * b.Z, a.W * b.W);
    public static Vector4 operator /(Vector4 value, float scalar) => Math.Abs(scalar) < Math.Epsilon ? Zero : new(value.X / scalar, value.Y / scalar, value.Z / scalar, value.W / scalar);
    public static bool operator ==(Vector4 a, Vector4 b) => a.Equals(b);
    public static bool operator !=(Vector4 a, Vector4 b) => !a.Equals(b);
}
