using System;
using System.Runtime.InteropServices;

namespace SFG;

[StructLayout(LayoutKind.Sequential)]
public struct Matrix4x3 : IEquatable<Matrix4x3>
{
    public static Matrix4x3 Identity => new(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);

    public float M00;
    public float M10;
    public float M20;
    public float M01;
    public float M11;
    public float M21;
    public float M02;
    public float M12;
    public float M22;
    public float M03;
    public float M13;
    public float M23;

    public Matrix4x3(float m00, float m10, float m20, float m01, float m11, float m21, float m02, float m12, float m22, float m03, float m13, float m23)
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
        M03 = m03;
        M13 = m13;
        M23 = m23;
    }

    public float this[int index]
    {
        readonly get => index switch
        {
            0 => M00, 1 => M10, 2 => M20, 3 => M01, 4 => M11, 5 => M21,
            6 => M02, 7 => M12, 8 => M22, 9 => M03, 10 => M13, 11 => M23,
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
                case 9: M03 = value; break;
                case 10: M13 = value; break;
                case 11: M23 = value; break;
                default: throw new IndexOutOfRangeException();
            }
        }
    }

    public readonly Vector3 Translation => new(M03, M13, M23);
    public readonly Vector3 Scale => new(new Vector3(M00, M10, M20).Length, new Vector3(M01, M11, M21).Length, new Vector3(M02, M12, M22).Length);

    public static Matrix4x3 CreateTranslation(Vector3 translation) => new(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, translation.X, translation.Y, translation.Z);
    public static Matrix4x3 CreateScale(Vector3 scale) => new(scale.X, 0.0f, 0.0f, 0.0f, scale.Y, 0.0f, 0.0f, 0.0f, scale.Z, 0.0f, 0.0f, 0.0f);

    public static Matrix4x3 CreateRotation(Quaternion rotation)
    {
        Matrix3x3 matrix = Matrix3x3.Rotation(rotation);
        return new Matrix4x3(matrix.M00, matrix.M10, matrix.M20, matrix.M01, matrix.M11, matrix.M21, matrix.M02, matrix.M12, matrix.M22, 0.0f, 0.0f, 0.0f);
    }

    public static Matrix4x3 CreateTransform(Vector3 position, Quaternion rotation, Vector3 scale)
    {
        return CreateTranslation(position) * CreateRotation(rotation) * CreateScale(scale);
    }

    public static Matrix4x3 FromMatrix4x4(Matrix4x4 matrix)
    {
        return new Matrix4x3(matrix.M00, matrix.M10, matrix.M20, matrix.M01, matrix.M11, matrix.M21, matrix.M02, matrix.M12, matrix.M22, matrix.M03, matrix.M13, matrix.M23);
    }

    public readonly Matrix4x3 Inversed
    {
        get
        {
            Vector3 scale = Scale;
            Vector3 inverseScaleSquared = new(1.0f / (scale.X * scale.X), 1.0f / (scale.Y * scale.Y), 1.0f / (scale.Z * scale.Z));
            Matrix4x3 result = new(M00 * inverseScaleSquared.X, M01 * inverseScaleSquared.X, M02 * inverseScaleSquared.X, M10 * inverseScaleSquared.Y, M11 * inverseScaleSquared.Y, M12 * inverseScaleSquared.Y, M20 * inverseScaleSquared.Z, M21 * inverseScaleSquared.Z, M22 * inverseScaleSquared.Z, 0.0f, 0.0f, 0.0f);
            Vector3 inverseTranslation = result.TransformDirection(Translation);
            result.M03 = -inverseTranslation.X;
            result.M13 = -inverseTranslation.Y;
            result.M23 = -inverseTranslation.Z;
            return result;
        }
    }

    public readonly void Decompose(out Vector3 position, out Quaternion rotation, out Vector3 scale)
    {
        position = Translation;
        scale = Scale;
        Vector3 x = scale.X == 0.0f ? Vector3.Zero : new Vector3(M00, M10, M20) / scale.X;
        Vector3 y = scale.Y == 0.0f ? Vector3.Zero : new Vector3(M01, M11, M21) / scale.Y;
        Vector3 z = scale.Z == 0.0f ? Vector3.Zero : new Vector3(M02, M12, M22) / scale.Z;
        rotation = Quaternion.FromRotationMatrix(Matrix3x3.FromAxes(x, y, z));
    }

    public readonly Vector3 GetColumn(int index) => index switch
    {
        0 => new Vector3(M00, M10, M20),
        1 => new Vector3(M01, M11, M21),
        2 => new Vector3(M02, M12, M22),
        3 => new Vector3(M03, M13, M23),
        _ => throw new IndexOutOfRangeException(),
    };

    public readonly Vector3 TransformDirection(Vector3 vector)
    {
        return new Vector3(M00 * vector.X + M01 * vector.Y + M02 * vector.Z, M10 * vector.X + M11 * vector.Y + M12 * vector.Z, M20 * vector.X + M21 * vector.Y + M22 * vector.Z);
    }

    public readonly Matrix4x4 ToMatrix4x4() => new(M00, M10, M20, 0.0f, M01, M11, M21, 0.0f, M02, M12, M22, 0.0f, M03, M13, M23, 1.0f);
    public readonly Matrix3x3 ToLinear3x3() => new(M00, M10, M20, M01, M11, M21, M02, M12, M22);

    public readonly bool ApproximatelyEquals(Matrix4x3 other, float epsilon = Math.Epsilon)
    {
        for (int index = 0; index < 12; ++index)
        {
            if (!Math.ApproximatelyEqual(this[index], other[index], epsilon))
            {
                return false;
            }
        }

        return true;
    }

    public readonly bool Equals(Matrix4x3 other)
    {
        for (int index = 0; index < 12; ++index)
        {
            if (this[index] != other[index])
            {
                return false;
            }
        }

        return true;
    }

    public override readonly bool Equals(object? obj) => obj is Matrix4x3 other && Equals(other);
    public override readonly int GetHashCode() => HashCode.Combine(HashCode.Combine(M00, M10, M20, M01), HashCode.Combine(M11, M21, M02, M12), HashCode.Combine(M22, M03, M13, M23));

    public static Matrix4x3 operator *(Matrix4x3 a, Matrix4x3 b)
    {
        Matrix4x3 result = default;

        for (int column = 0; column < 3; ++column)
        {
            for (int row = 0; row < 3; ++row)
            {
                result[column * 3 + row] = a[row] * b[column * 3] + a[3 + row] * b[column * 3 + 1] + a[6 + row] * b[column * 3 + 2];
            }
        }

        for (int row = 0; row < 3; ++row)
        {
            result[9 + row] = a[row] * b.M03 + a[3 + row] * b.M13 + a[6 + row] * b.M23 + a[9 + row];
        }

        return result;
    }

    public static Vector3 operator *(Matrix4x3 matrix, Vector3 vector)
    {
        return matrix.TransformDirection(vector) + matrix.Translation;
    }

    public static Matrix4x3 operator *(Matrix4x3 matrix, float scalar)
    {
        Matrix4x3 result = default;

        for (int index = 0; index < 12; ++index)
        {
            result[index] = matrix[index] * scalar;
        }

        return result;
    }

    public static bool operator ==(Matrix4x3 a, Matrix4x3 b) => a.Equals(b);
    public static bool operator !=(Matrix4x3 a, Matrix4x3 b) => !a.Equals(b);
}
