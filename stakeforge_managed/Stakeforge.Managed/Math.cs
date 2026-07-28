using System;
using System.Numerics;

namespace SFG;

public static class Math
{
    public const float DegreesToRadiansFactor = 0.0174533f;
    public const float RadiansToDegreesFactor = 57.2958f;
    public const float Pi = 3.1415926535897932f;
    public const float TwoPi = 6.28318530717959f;
    public const float HalfPi = 1.57079632679f;
    public const float E = 2.71828182845904523536f;
    public const float Epsilon = 0.00001f;

    public static bool IsNaN(float value) => float.IsNaN(value);
    public static float CopySign(float value, float sign) => MathF.CopySign(value, sign);
    public static float Round(float value) => MathF.Round(value);
    public static float Ceiling(float value) => MathF.Ceiling(value);
    public static float Floor(float value) => MathF.Floor(value);
    public static float Acos(float value) => MathF.Acos(value);
    public static float Asin(float value) => MathF.Asin(value);
    public static float Atan2(float y, float x) => MathF.Atan2(y, x);
    public static float Clamp(float value, float minimum, float maximum) => MathF.Max(minimum, MathF.Min(value, maximum));
    public static int Clamp(int value, int minimum, int maximum) => System.Math.Max(minimum, System.Math.Min(value, maximum));
    public static float Max(float a, float b) => MathF.Max(a, b);
    public static int Max(int a, int b) => System.Math.Max(a, b);
    public static float Min(float a, float b) => MathF.Min(a, b);
    public static int Min(int a, int b) => System.Math.Min(a, b);
    public static float Abs(float value) => MathF.Abs(value);
    public static int Abs(int value) => System.Math.Abs(value);
    public static float Pow(float value, float exponent) => MathF.Pow(value, exponent);
    public static float Sqrt(float value) => MathF.Sqrt(value);
    public static float Sin(float radians) => MathF.Sin(radians);
    public static float Cos(float radians) => MathF.Cos(radians);
    public static float Tan(float radians) => MathF.Tan(radians);
    public static float Mod(float value, float modulus) => value % modulus;
    public static float DegreesToRadians(float degrees) => degrees * DegreesToRadiansFactor;
    public static float RadiansToDegrees(float radians) => radians * RadiansToDegreesFactor;
    public static bool ApproximatelyEqual(float a, float b, float epsilon = Epsilon) => Abs(a - b) < epsilon;
    public static int Sign(float value) => value > 0.0f ? 1 : value < 0.0f ? -1 : 0;
    public static float Lerp(float a, float b, float amount) => a + amount * (b - a);

    public static float InverseLerp(float a, float b, float value)
    {
        return Abs(b - a) < Epsilon ? 0.0f : (value - a) / (b - a);
    }

    public static float Remap(float value, float inputMinimum, float inputMaximum, float outputMinimum, float outputMaximum)
    {
        if (Abs(inputMaximum - inputMinimum) < Epsilon)
        {
            return outputMinimum;
        }

        float normalized = (value - inputMinimum) / (inputMaximum - inputMinimum);
        return outputMinimum + normalized * (outputMaximum - outputMinimum);
    }

    public static uint FloorLog2(uint value)
    {
        return value == 0 ? 0 : (uint)BitOperations.Log2(value);
    }

    public static double FastPow(double value, double exponent)
    {
        long bits = BitConverter.DoubleToInt64Bits(value);
        int upper = (int)(bits >> 32);
        int approximatedUpper = unchecked((int)(exponent * (upper - 1072632447) + 1072632447));
        return BitConverter.Int64BitsToDouble((long)(uint)approximatedUpper << 32);
    }
}
