namespace SFG;

public static class Ease
{
    public static float SmoothDamp(float current, float target, ref float currentVelocity, float smoothTime, float maximumSpeed, float deltaTime)
    {
        smoothTime = Math.Max(0.0001f, smoothTime);
        float frequency = 2.0f / smoothTime;
        float step = frequency * deltaTime;
        float decay = 1.0f / (1.0f + step + 0.48f * step * step + 0.235f * step * step * step);
        float change = current - target;
        float originalTarget = target;
        float maximumChange = maximumSpeed * smoothTime;
        change = Math.Clamp(change, -maximumChange, maximumChange);
        target = current - change;
        float velocityStep = (currentVelocity + frequency * change) * deltaTime;
        currentVelocity = (currentVelocity - frequency * velocityStep) * decay;
        float result = target + (change + velocityStep) * decay;

        if ((originalTarget - current > 0.0f) == (result > originalTarget))
        {
            result = originalTarget;
            currentVelocity = (result - originalTarget) / deltaTime;
        }

        return result;
    }

    public static float Lerp(float a, float b, float amount) => a * (1.0f - amount) + b * amount;
    public static float CubicLerp(float a, float b, float amount) => Lerp(a, b, 3.0f * amount * amount - 2.0f * amount * amount * amount);

    public static float CubicInterpolate(float value0, float value1, float value2, float value3, float amount)
    {
        float amountSquared = amount * amount;
        return (value3 * 0.5f - value2 * 1.5f - value0 * 0.5f + value1 * 1.5f) * amount * amountSquared +
               (value0 - value1 * 2.5f + value2 * 2.0f - value3 * 0.5f) * amountSquared +
               (value2 * 0.5f - value0 * 0.5f) * amount +
               value1;
    }

    public static float CubicInterpolateTangents(float value1, float tangent1, float value2, float tangent2, float amount)
    {
        float amountSquared = amount * amount;
        return (tangent2 - value2 * 2.0f + tangent1 + value1 * 2.0f) * amount * amountSquared +
               (tangent1 * 2.0f - value1 * 3.0f + value2 * 3.0f - tangent2 * 2.0f) * amountSquared +
               tangent1 * amount +
               value1;
    }

    public static float Bilerp(float value00, float value10, float value01, float value11, float amountX, float amountY)
    {
        return Lerp(Lerp(value00, value10, amountX), Lerp(value01, value11, amountX), amountY);
    }

    public static float Step(float edge, float value) => value < edge ? 0.0f : 1.0f;
    public static float In(float start, float end, float amount) => Lerp(start, end, amount * amount);
    public static float Out(float start, float end, float amount) => Lerp(start, end, 1.0f - (1.0f - amount) * (1.0f - amount));

    public static float InOut(float start, float end, float amount)
    {
        return amount < 0.5f ? Lerp(start, end, 2.0f * amount * amount) : Lerp(start, end, 1.0f - (float)Math.FastPow(-2.0f * amount + 2.0f, 2.0f) * 0.5f);
    }

    public static float Cubic(float start, float end, float amount) => Lerp(start, end, amount * amount * amount);

    public static float Exponential(float start, float end, float amount)
    {
        return Math.Abs(amount) < 0.001f ? 0.0f : Lerp(start, end, (float)Math.FastPow(2.0f, 10.0f * amount - 10.0f));
    }

    public static float Bounce(float start, float end, float amount)
    {
        if (amount < 1.0f / 2.75f)
        {
            return Lerp(start, end, 7.5625f * amount * amount);
        }

        if (amount < 2.0f / 2.75f)
        {
            amount -= 1.5f / 2.75f;
            return Lerp(start, end, 7.5625f * amount * amount + 0.75f);
        }

        if (amount < 2.5f / 2.75f)
        {
            amount -= 2.25f / 2.75f;
            return Lerp(start, end, 7.5625f * amount * amount + 0.9375f);
        }

        amount -= 2.625f / 2.75f;
        return Lerp(start, end, 7.5625f * amount * amount + 0.984375f);
    }

    public static float Sinusoidal(float start, float end, float amount)
    {
        return Lerp(start, end, -Math.Cos(amount * Math.Pi) * 0.5f + 0.5f);
    }
}
