using System;
using System.Runtime.InteropServices;

namespace SFG;

[StructLayout(LayoutKind.Sequential)]
public struct Quaternion : IEquatable<Quaternion>
{
    public static Quaternion Identity => new(0.0f, 0.0f, 0.0f, 1.0f);

    public float X;
    public float Y;
    public float Z;
    public float W;

    public Quaternion(float x, float y, float z, float w)
    {
        X = x;
        Y = y;
        Z = z;
        W = w;
    }

    public readonly Vector3 Right => this * Vector3.Right;
    public readonly Vector3 Up => this * Vector3.Up;
    public readonly Vector3 Forward => this * Vector3.Forward;
    public readonly float Length => Math.Sqrt(LengthSquared);
    public readonly float LengthSquared => X * X + Y * Y + Z * Z + W * W;
    public readonly float Magnitude => Length;
    public readonly float MagnitudeSquared => LengthSquared;

    public readonly Quaternion Normalized
    {
        get
        {
            float length = Length;
            return length > Math.Epsilon ? this / length : Identity;
        }
    }
    public readonly Quaternion Conjugate => new(-X, -Y, -Z, W);

    public readonly Quaternion Inverse
    {
        get
        {
            float lengthSquared = LengthSquared;
            return Math.Abs(lengthSquared) < Math.Epsilon ? Identity : Conjugate / lengthSquared;
        }
    }

    public void Normalize()
    {
        this = Normalized;
    }

    public static float Dot(Quaternion a, Quaternion b)
    {
        return a.X * b.X + a.Y * b.Y + a.Z * b.Z + a.W * b.W;
    }

    public static Quaternion FromEuler(float pitchDegrees, float yawDegrees, float rollDegrees)
    {
        float pitch = Math.DegreesToRadians(pitchDegrees);
        float yaw = Math.DegreesToRadians(yawDegrees);
        float roll = Math.DegreesToRadians(rollDegrees);
        float cx = Math.Cos(pitch * 0.5f);
        float sx = Math.Sin(pitch * 0.5f);
        float cy = Math.Cos(yaw * 0.5f);
        float sy = Math.Sin(yaw * 0.5f);
        float cz = Math.Cos(roll * 0.5f);
        float sz = Math.Sin(roll * 0.5f);

        return new Quaternion(
            sx * cy * cz - cx * sy * sz,
            cx * sy * cz + sx * cy * sz,
            cx * cy * sz - sx * sy * cz,
            cx * cy * cz + sx * sy * sz);
    }

    public static Vector3 ToEuler(Quaternion rotation)
    {
        float sinPitch = 2.0f * (rotation.W * rotation.X + rotation.Y * rotation.Z);
        float cosPitch = 1.0f - 2.0f * (rotation.X * rotation.X + rotation.Y * rotation.Y);
        float sinYaw = Math.Clamp(2.0f * (rotation.W * rotation.Y - rotation.Z * rotation.X), -1.0f, 1.0f);
        float sinRoll = 2.0f * (rotation.W * rotation.Z + rotation.X * rotation.Y);
        float cosRoll = 1.0f - 2.0f * (rotation.Y * rotation.Y + rotation.Z * rotation.Z);

        return new Vector3(
            Math.RadiansToDegrees(Math.Atan2(sinPitch, cosPitch)),
            Math.RadiansToDegrees(Math.Asin(sinYaw)),
            Math.RadiansToDegrees(Math.Atan2(sinRoll, cosRoll)));
    }

    public static Quaternion AngleAxis(float angleDegrees, Vector3 axis)
    {
        Vector3 normalizedAxis = axis.Normalized;
        float halfAngle = Math.DegreesToRadians(angleDegrees * 0.5f);
        float sine = Math.Sin(halfAngle);
        return new Quaternion(normalizedAxis.X * sine, normalizedAxis.Y * sine, normalizedAxis.Z * sine, Math.Cos(halfAngle));
    }

    public static Quaternion Lerp(Quaternion a, Quaternion b, float amount)
    {
        Quaternion adjusted = Dot(a, b) < 0.0f ? -b : b;
        return (a * (1.0f - amount) + adjusted * amount).Normalized;
    }

    public static Quaternion Slerp(Quaternion a, Quaternion b, float amount)
    {
        float dot = Dot(a, b);
        Quaternion adjusted = b;

        if (dot < 0.0f)
        {
            dot = -dot;
            adjusted = -b;
        }

        dot = Math.Clamp(dot, -1.0f, 1.0f);

        if (dot > 0.9995f)
        {
            return Lerp(a, adjusted, amount);
        }

        float angle = Math.Acos(dot);
        float sine = Math.Sin(angle);
        float aWeight = Math.Sin((1.0f - amount) * angle) / sine;
        float bWeight = Math.Sin(amount * angle) / sine;
        return (a * aWeight + adjusted * bWeight).Normalized;
    }

    public static Quaternion LookAt(Vector3 source, Vector3 target, Vector3 up)
    {
        Vector3 forward = (target - source).Normalized;

        if (forward.IsZero())
        {
            return Identity;
        }

        Vector3 right = Vector3.Cross(forward, up).Normalized;

        if (right.IsZero())
        {
            return Identity;
        }

        Vector3 finalUp = Vector3.Cross(right, forward);
        return FromRotationMatrix(Matrix3x3.FromAxes(right, finalUp, -forward));
    }

    public static Quaternion FromRotationMatrix(Matrix3x3 matrix)
    {
        float trace = matrix.M00 + matrix.M11 + matrix.M22;
        Quaternion result = Identity;

        if (trace > 0.0f)
        {
            float scale = Math.Sqrt(trace + 1.0f) * 2.0f;
            result.W = 0.25f * scale;
            result.X = (matrix.M21 - matrix.M12) / scale;
            result.Y = (matrix.M02 - matrix.M20) / scale;
            result.Z = (matrix.M10 - matrix.M01) / scale;
        }
        else if (matrix.M00 > matrix.M11 && matrix.M00 > matrix.M22)
        {
            float scale = Math.Sqrt(1.0f + matrix.M00 - matrix.M11 - matrix.M22) * 2.0f;
            result.W = (matrix.M21 - matrix.M12) / scale;
            result.X = 0.25f * scale;
            result.Y = (matrix.M01 + matrix.M10) / scale;
            result.Z = (matrix.M02 + matrix.M20) / scale;
        }
        else if (matrix.M11 > matrix.M22)
        {
            float scale = Math.Sqrt(1.0f + matrix.M11 - matrix.M00 - matrix.M22) * 2.0f;
            result.W = (matrix.M02 - matrix.M20) / scale;
            result.X = (matrix.M01 + matrix.M10) / scale;
            result.Y = 0.25f * scale;
            result.Z = (matrix.M12 + matrix.M21) / scale;
        }
        else
        {
            float scale = Math.Sqrt(1.0f + matrix.M22 - matrix.M00 - matrix.M11) * 2.0f;
            result.W = (matrix.M10 - matrix.M01) / scale;
            result.X = (matrix.M02 + matrix.M20) / scale;
            result.Y = (matrix.M12 + matrix.M21) / scale;
            result.Z = 0.25f * scale;
        }

        return result.Normalized;
    }

    public readonly bool IsIdentity(float epsilon = Math.Epsilon) => ApproximatelyEquals(Identity, epsilon);

    public readonly bool ApproximatelyEquals(Quaternion other, float epsilon = Math.Epsilon)
    {
        return Math.ApproximatelyEqual(X, other.X, epsilon) &&
               Math.ApproximatelyEqual(Y, other.Y, epsilon) &&
               Math.ApproximatelyEqual(Z, other.Z, epsilon) &&
               Math.ApproximatelyEqual(W, other.W, epsilon);
    }

    public readonly bool Equals(Quaternion other) => X.Equals(other.X) && Y.Equals(other.Y) && Z.Equals(other.Z) && W.Equals(other.W);
    public override readonly bool Equals(object? obj) => obj is Quaternion other && Equals(other);
    public override readonly int GetHashCode() => HashCode.Combine(X, Y, Z, W);
    public override readonly string ToString() => $"({X}, {Y}, {Z}, {W})";

    public static Quaternion operator +(Quaternion a, Quaternion b) => new(a.X + b.X, a.Y + b.Y, a.Z + b.Z, a.W + b.W);
    public static Quaternion operator -(Quaternion a, Quaternion b) => new(a.X - b.X, a.Y - b.Y, a.Z - b.Z, a.W - b.W);
    public static Quaternion operator -(Quaternion value) => new(-value.X, -value.Y, -value.Z, -value.W);
    public static Quaternion operator *(Quaternion a, Quaternion b) => new(a.W * b.X + a.X * b.W + a.Y * b.Z - a.Z * b.Y, a.W * b.Y + a.Y * b.W + a.Z * b.X - a.X * b.Z, a.W * b.Z + a.Z * b.W + a.X * b.Y - a.Y * b.X, a.W * b.W - a.X * b.X - a.Y * b.Y - a.Z * b.Z);
    public static Quaternion operator *(Quaternion value, float scalar) => new(value.X * scalar, value.Y * scalar, value.Z * scalar, value.W * scalar);
    public static Quaternion operator *(float scalar, Quaternion value) => value * scalar;
    public static Quaternion operator /(Quaternion value, float scalar) => Math.Abs(scalar) < Math.Epsilon ? Identity : new(value.X / scalar, value.Y / scalar, value.Z / scalar, value.W / scalar);

    public static Vector3 operator *(Quaternion rotation, Vector3 vector)
    {
        Quaternion point = new(vector.X, vector.Y, vector.Z, 0.0f);
        Quaternion rotated = rotation * point * rotation.Conjugate;
        return new Vector3(rotated.X, rotated.Y, rotated.Z);
    }

    public static bool operator ==(Quaternion a, Quaternion b) => a.Equals(b);
    public static bool operator !=(Quaternion a, Quaternion b) => !a.Equals(b);
}
