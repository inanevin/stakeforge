using System;
using System.Runtime.InteropServices;

namespace SFG;

[StructLayout(LayoutKind.Sequential)]
public struct Matrix3x3 : IEquatable<Matrix3x3>
{
    public static Matrix3x3 Identity => new(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

    public float M00;
    public float M10;
    public float M20;
    public float M01;
    public float M11;
    public float M21;
    public float M02;
    public float M12;
    public float M22;

    public Matrix3x3(float m00, float m10, float m20, float m01, float m11, float m21, float m02, float m12, float m22)
    {
        M00 = m00;
        M10 = m10;
        M20 = m20;
        M01 = m01;
        M11 = m11;
        M21 = m21;
        M02 = m02;
        M12 = m12;
        M22 = m22;
    }

    public float this[int index]
    {
        readonly get => index switch
        {
            0 => M00,
            1 => M10,
            2 => M20,
            3 => M01,
            4 => M11,
            5 => M21,
            6 => M02,
            7 => M12,
            8 => M22,
            _ => throw new IndexOutOfRangeException(),
        };
        set
        {
            switch (index)
            {
                case 0: M00 = value; break;
                case 1: M10 = value; break;
                case 2: M20 = value; break;
                case 3: M01 = value; break;
                case 4: M11 = value; break;
                case 5: M21 = value; break;
                case 6: M02 = value; break;
                case 7: M12 = value; break;
                case 8: M22 = value; break;
                default: throw new IndexOutOfRangeException();
            }
        }
    }

    public readonly float Determinant
    {
        get
        {
            return M00 * (M11 * M22 - M21 * M12) -
                   M01 * (M10 * M22 - M20 * M12) +
                   M02 * (M10 * M21 - M20 * M11);
        }
    }

    public readonly Matrix3x3 Transposed => new(M00, M01, M02, M10, M11, M12, M20, M21, M22);

    public readonly Matrix3x3 Inversed
    {
        get
        {
            float a = M11 * M22 - M21 * M12;
            float b = -(M01 * M22 - M21 * M02);
            float c = M01 * M12 - M11 * M02;
            float d = -(M10 * M22 - M20 * M12);
            float e = M00 * M22 - M20 * M02;
            float f = -(M00 * M12 - M10 * M02);
            float g = M10 * M21 - M20 * M11;
            float h = -(M00 * M21 - M20 * M01);
            float i = M00 * M11 - M10 * M01;
            float determinant = M00 * a + M01 * d + M02 * g;

            if (Math.Abs(determinant) <= 0.00000001f)
            {
                return Identity;
            }

            float inverseDeterminant = 1.0f / determinant;
            return new Matrix3x3(a * inverseDeterminant, d * inverseDeterminant, g * inverseDeterminant, b * inverseDeterminant, e * inverseDeterminant, h * inverseDeterminant, c * inverseDeterminant, f * inverseDeterminant, i * inverseDeterminant);
        }
    }

    public static Matrix3x3 Scale(Vector3 scale) => new(scale.X, 0.0f, 0.0f, 0.0f, scale.Y, 0.0f, 0.0f, 0.0f, scale.Z);

    public static Matrix3x3 Rotation(Quaternion rotation)
    {
        float x2 = rotation.X * rotation.X;
        float y2 = rotation.Y * rotation.Y;
        float z2 = rotation.Z * rotation.Z;
        float xy = rotation.X * rotation.Y;
        float xz = rotation.X * rotation.Z;
        float yz = rotation.Y * rotation.Z;
        float wx = rotation.W * rotation.X;
        float wy = rotation.W * rotation.Y;
        float wz = rotation.W * rotation.Z;
        return new Matrix3x3(1.0f - 2.0f * (y2 + z2), 2.0f * (xy + wz), 2.0f * (xz - wy), 2.0f * (xy - wz), 1.0f - 2.0f * (x2 + z2), 2.0f * (yz + wx), 2.0f * (xz + wy), 2.0f * (yz - wx), 1.0f - 2.0f * (x2 + y2));
    }

    public static Matrix3x3 FromAxes(Vector3 x, Vector3 y, Vector3 z) => new(x.X, x.Y, x.Z, y.X, y.Y, y.Z, z.X, z.Y, z.Z);

    public static Matrix3x3 Abs(Matrix3x3 matrix)
    {
        return new Matrix3x3(Math.Abs(matrix.M00), Math.Abs(matrix.M10), Math.Abs(matrix.M20), Math.Abs(matrix.M01), Math.Abs(matrix.M11), Math.Abs(matrix.M21), Math.Abs(matrix.M02), Math.Abs(matrix.M12), Math.Abs(matrix.M22));
    }

    public readonly Vector3 GetColumn(int index) => index switch
    {
        0 => new Vector3(M00, M10, M20),
        1 => new Vector3(M01, M11, M21),
        2 => new Vector3(M02, M12, M22),
        _ => throw new IndexOutOfRangeException(),
    };

    public readonly Vector3 GetRow(int index) => index switch
    {
        0 => new Vector3(M00, M01, M02),
        1 => new Vector3(M10, M11, M12),
        2 => new Vector3(M20, M21, M22),
        _ => throw new IndexOutOfRangeException(),
    };

    public readonly Matrix4x4 ToMatrix4x4() => new(M00, M10, M20, 0.0f, M01, M11, M21, 0.0f, M02, M12, M22, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

    public readonly bool ApproximatelyEquals(Matrix3x3 other, float epsilon = Math.Epsilon)
    {
        for (int index = 0; index < 9; ++index)
        {
            if (!Math.ApproximatelyEqual(this[index], other[index], epsilon))
            {
                return false;
            }
        }

        return true;
    }

    public readonly bool Equals(Matrix3x3 other)
    {
        for (int index = 0; index < 9; ++index)
        {
            if (this[index] != other[index])
            {
                return false;
            }
        }

        return true;
    }

    public override readonly bool Equals(object? obj) => obj is Matrix3x3 other && Equals(other);
    public override readonly int GetHashCode() => HashCode.Combine(HashCode.Combine(M00, M10, M20, M01), HashCode.Combine(M11, M21, M02, M12), M22);

    public static Vector3 operator *(Matrix3x3 matrix, Vector3 vector)
    {
        return new Vector3(matrix.M00 * vector.X + matrix.M01 * vector.Y + matrix.M02 * vector.Z, matrix.M10 * vector.X + matrix.M11 * vector.Y + matrix.M12 * vector.Z, matrix.M20 * vector.X + matrix.M21 * vector.Y + matrix.M22 * vector.Z);
    }

    public static Matrix3x3 operator *(Matrix3x3 a, Matrix3x3 b)
    {
        Matrix3x3 result = default;

        for (int column = 0; column < 3; ++column)
        {
            for (int row = 0; row < 3; ++row)
            {
                result[column * 3 + row] = a[row] * b[column * 3] + a[3 + row] * b[column * 3 + 1] + a[6 + row] * b[column * 3 + 2];
            }
        }

        return result;
    }

    public static Matrix3x3 operator *(Matrix3x3 matrix, float scalar)
    {
        Matrix3x3 result = default;

        for (int index = 0; index < 9; ++index)
        {
            result[index] = matrix[index] * scalar;
        }

        return result;
    }

    public static bool operator ==(Matrix3x3 a, Matrix3x3 b) => a.Equals(b);
    public static bool operator !=(Matrix3x3 a, Matrix3x3 b) => !a.Equals(b);
}
