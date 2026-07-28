using System;
using System.Runtime.InteropServices;

namespace SFG;

[StructLayout(LayoutKind.Sequential)]
public struct Matrix4x4 : IEquatable<Matrix4x4>
{
    public static Matrix4x4 Identity => new(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

    public float M00;
    public float M10;
    public float M20;
    public float M30;
    public float M01;
    public float M11;
    public float M21;
    public float M31;
    public float M02;
    public float M12;
    public float M22;
    public float M32;
    public float M03;
    public float M13;
    public float M23;
    public float M33;

    public Matrix4x4(float m00, float m10, float m20, float m30, float m01, float m11, float m21, float m31, float m02, float m12, float m22, float m32, float m03, float m13, float m23, float m33)
    {
        M00 = m00;
        M10 = m10;
        M20 = m20;
        M30 = m30;
        M01 = m01;
        M11 = m11;
        M21 = m21;
        M31 = m31;
        M02 = m02;
        M12 = m12;
        M22 = m22;
        M32 = m32;
        M03 = m03;
        M13 = m13;
        M23 = m23;
        M33 = m33;
    }

    public float this[int index]
    {
        readonly get => index switch
        {
            0 => M00, 1 => M10, 2 => M20, 3 => M30,
            4 => M01, 5 => M11, 6 => M21, 7 => M31,
            8 => M02, 9 => M12, 10 => M22, 11 => M32,
            12 => M03, 13 => M13, 14 => M23, 15 => M33,
            _ => throw new IndexOutOfRangeException(),
        };
        set
        {
            switch (index)
            {
                case 0: M00 = value; break;
                case 1: M10 = value; break;
                case 2: M20 = value; break;
                case 3: M30 = value; break;
                case 4: M01 = value; break;
                case 5: M11 = value; break;
                case 6: M21 = value; break;
                case 7: M31 = value; break;
                case 8: M02 = value; break;
                case 9: M12 = value; break;
                case 10: M22 = value; break;
                case 11: M32 = value; break;
                case 12: M03 = value; break;
                case 13: M13 = value; break;
                case 14: M23 = value; break;
                case 15: M33 = value; break;
                default: throw new IndexOutOfRangeException();
            }
        }
    }

    public readonly Vector3 Translation => new(M03, M13, M23);
    public readonly Vector3 Scale => new(new Vector3(M00, M10, M20).Length, new Vector3(M01, M11, M21).Length, new Vector3(M02, M12, M22).Length);

    public readonly Matrix4x4 Transposed => new(M00, M01, M02, M03, M10, M11, M12, M13, M20, M21, M22, M23, M30, M31, M32, M33);

    public readonly float Determinant
    {
        get
        {
            float kpMinusLo = M22 * M33 - M23 * M32;
            float jpMinusLn = M21 * M33 - M23 * M31;
            float joMinusKn = M21 * M32 - M22 * M31;
            float ipMinusLm = M20 * M33 - M23 * M30;
            float ioMinusKm = M20 * M32 - M22 * M30;
            float inMinusJm = M20 * M31 - M21 * M30;

            return M00 * (M11 * kpMinusLo - M12 * jpMinusLn + M13 * joMinusKn) -
                   M01 * (M10 * kpMinusLo - M12 * ipMinusLm + M13 * ioMinusKm) +
                   M02 * (M10 * jpMinusLn - M11 * ipMinusLm + M13 * inMinusJm) -
                   M03 * (M10 * joMinusKn - M11 * ioMinusKm + M12 * inMinusJm);
        }
    }

    public readonly Matrix4x4 Inversed
    {
        get
        {
            if (Math.Abs(Determinant) < Math.Epsilon)
            {
                return Identity;
            }

            System.Numerics.Matrix4x4 matrix = ToNumerics();
            return System.Numerics.Matrix4x4.Invert(matrix, out System.Numerics.Matrix4x4 inverse) ? FromNumerics(inverse) : Identity;
        }
    }

    public readonly Matrix4x4 NormalMatrix => Inversed.Transposed;

    public static Matrix4x4 CreateTranslation(Vector3 translation)
    {
        return new Matrix4x4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, translation.X, translation.Y, translation.Z, 1.0f);
    }

    public static Matrix4x4 CreateScale(Vector3 scale)
    {
        return new Matrix4x4(scale.X, 0.0f, 0.0f, 0.0f, 0.0f, scale.Y, 0.0f, 0.0f, 0.0f, 0.0f, scale.Z, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    }

    public static Matrix4x4 CreateRotation(Quaternion rotation)
    {
        return Matrix3x3.Rotation(rotation).ToMatrix4x4();
    }

    public static Matrix4x4 CreateTransform(Vector3 position, Quaternion rotation, Vector3 scale)
    {
        return CreateTranslation(position) * CreateRotation(rotation) * CreateScale(scale);
    }

    public static Matrix4x4 CreateOrthographicReverseZ(float left, float right, float top, float bottom, float nearPlane, float farPlane)
    {
        float inverseWidth = 1.0f / (right - left);
        float inverseHeight = 1.0f / (top - bottom);
        float inverseDepth = 1.0f / (farPlane - nearPlane);
        return new Matrix4x4(2.0f * inverseWidth, 0.0f, 0.0f, 0.0f, 0.0f, 2.0f * inverseHeight, 0.0f, 0.0f, 0.0f, 0.0f, -inverseDepth, 0.0f, -(right + left) * inverseWidth, -(top + bottom) * inverseHeight, 1.0f + nearPlane * inverseDepth, 1.0f);
    }

    public static Matrix4x4 CreateOrthographic(float left, float right, float top, float bottom, float nearPlane, float farPlane)
    {
        float inverseWidth = 1.0f / (right - left);
        float inverseHeight = 1.0f / (top - bottom);
        float inverseDepth = 1.0f / (nearPlane - farPlane);
        return new Matrix4x4(2.0f * inverseWidth, 0.0f, 0.0f, 0.0f, 0.0f, 2.0f * inverseHeight, 0.0f, 0.0f, 0.0f, 0.0f, inverseDepth, 0.0f, -(right + left) * inverseWidth, -(top + bottom) * inverseHeight, nearPlane * inverseDepth, 1.0f);
    }

    public static Matrix4x4 CreatePerspectiveReverseZ(float verticalFieldOfViewDegrees, float aspectRatio, float nearPlane, float farPlane)
    {
        float factor = 1.0f / Math.Tan(0.5f * Math.DegreesToRadians(verticalFieldOfViewDegrees));
        float inverseDepth = 1.0f / (farPlane - nearPlane);
        return new Matrix4x4(factor / aspectRatio, 0.0f, 0.0f, 0.0f, 0.0f, factor, 0.0f, 0.0f, 0.0f, 0.0f, nearPlane * inverseDepth, -1.0f, 0.0f, 0.0f, nearPlane * farPlane * inverseDepth, 0.0f);
    }

    public static Matrix4x4 CreatePerspective(float verticalFieldOfViewDegrees, float aspectRatio, float nearPlane, float farPlane)
    {
        float factor = 1.0f / Math.Tan(0.5f * Math.DegreesToRadians(verticalFieldOfViewDegrees));
        float inverseDepth = 1.0f / (nearPlane - farPlane);
        return new Matrix4x4(factor / aspectRatio, 0.0f, 0.0f, 0.0f, 0.0f, factor, 0.0f, 0.0f, 0.0f, 0.0f, farPlane * inverseDepth, -1.0f, 0.0f, 0.0f, nearPlane * farPlane * inverseDepth, 0.0f);
    }

    public static Matrix4x4 CreateLookAt(Vector3 eye, Vector3 target, Vector3 up)
    {
        Vector3 z = (eye - target).Normalized;
        Vector3 x = Vector3.Cross(up, z).Normalized;
        Vector3 y = Vector3.Cross(z, x);
        return new Matrix4x4(x.X, y.X, z.X, 0.0f, x.Y, y.Y, z.Y, 0.0f, x.Z, y.Z, z.Z, 0.0f, -Vector3.Dot(x, eye), -Vector3.Dot(y, eye), -Vector3.Dot(z, eye), 1.0f);
    }

    public static Matrix4x4 CreateView(Quaternion rotation, Vector3 position)
    {
        return CreateRotation(rotation.Inverse) * CreateTranslation(-position);
    }

    public readonly bool ApproximatelyEquals(Matrix4x4 other, float epsilon = Math.Epsilon)
    {
        for (int index = 0; index < 16; ++index)
        {
            if (!Math.ApproximatelyEqual(this[index], other[index], epsilon))
            {
                return false;
            }
        }

        return true;
    }

    public readonly bool Equals(Matrix4x4 other)
    {
        for (int index = 0; index < 16; ++index)
        {
            if (this[index] != other[index])
            {
                return false;
            }
        }

        return true;
    }

    public override readonly bool Equals(object? obj) => obj is Matrix4x4 other && Equals(other);
    public override readonly int GetHashCode() => HashCode.Combine(HashCode.Combine(M00, M10, M20, M30), HashCode.Combine(M01, M11, M21, M31), HashCode.Combine(M02, M12, M22, M32), HashCode.Combine(M03, M13, M23, M33));

    public static Matrix4x4 operator *(Matrix4x4 a, Matrix4x4 b)
    {
        return FromNumerics(a.ToNumerics() * b.ToNumerics());
    }

    public static Vector4 operator *(Matrix4x4 matrix, Vector4 vector)
    {
        return new Vector4(matrix.M00 * vector.X + matrix.M01 * vector.Y + matrix.M02 * vector.Z + matrix.M03 * vector.W, matrix.M10 * vector.X + matrix.M11 * vector.Y + matrix.M12 * vector.Z + matrix.M13 * vector.W, matrix.M20 * vector.X + matrix.M21 * vector.Y + matrix.M22 * vector.Z + matrix.M23 * vector.W, matrix.M30 * vector.X + matrix.M31 * vector.Y + matrix.M32 * vector.Z + matrix.M33 * vector.W);
    }

    public static Vector3 operator *(Matrix4x4 matrix, Vector3 vector)
    {
        Vector4 transformed = matrix * new Vector4(vector.X, vector.Y, vector.Z, 1.0f);
        return Math.Abs(transformed.W) < Math.Epsilon ? new Vector3(transformed.X, transformed.Y, transformed.Z) : new Vector3(transformed.X / transformed.W, transformed.Y / transformed.W, transformed.Z / transformed.W);
    }

    public static Matrix4x4 operator *(Matrix4x4 matrix, float scalar)
    {
        Matrix4x4 result = default;

        for (int index = 0; index < 16; ++index)
        {
            result[index] = matrix[index] * scalar;
        }

        return result;
    }

    public static Matrix4x4 operator /(Matrix4x4 matrix, float scalar) => Math.Abs(scalar) < Math.Epsilon ? Identity : matrix * (1.0f / scalar);
    public static bool operator ==(Matrix4x4 a, Matrix4x4 b) => a.Equals(b);
    public static bool operator !=(Matrix4x4 a, Matrix4x4 b) => !a.Equals(b);

    private readonly System.Numerics.Matrix4x4 ToNumerics()
    {
        return new System.Numerics.Matrix4x4(M00, M01, M02, M03, M10, M11, M12, M13, M20, M21, M22, M23, M30, M31, M32, M33);
    }

    private static Matrix4x4 FromNumerics(System.Numerics.Matrix4x4 matrix)
    {
        return new Matrix4x4(matrix.M11, matrix.M21, matrix.M31, matrix.M41, matrix.M12, matrix.M22, matrix.M32, matrix.M42, matrix.M13, matrix.M23, matrix.M33, matrix.M43, matrix.M14, matrix.M24, matrix.M34, matrix.M44);
    }
}
