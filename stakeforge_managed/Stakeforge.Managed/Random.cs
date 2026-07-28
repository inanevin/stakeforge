using System;

namespace SFG;

public struct RandomState
{
    public uint State;

    public RandomState(uint seed)
    {
        State = seed;
    }

    public uint NextUInt()
    {
        State ^= State >> 16;
        State = unchecked(State * 0x7feb352dU);
        State ^= State >> 15;
        State = unchecked(State * 0x846ca68bU);
        State ^= State >> 16;
        return State;
    }

    public float Float01() => (NextUInt() >> 8) * (1.0f / 16777216.0f);
    public float Range(float minimum, float maximum) => minimum + (maximum - minimum) * Float01();

    public int Range(int minimumInclusive, int maximumInclusive)
    {
        if (minimumInclusive > maximumInclusive)
        {
            throw new ArgumentOutOfRangeException(nameof(minimumInclusive));
        }

        ulong range = (ulong)((long)maximumInclusive - minimumInclusive + 1L);
        return (int)(minimumInclusive + (long)(NextUInt() % range));
    }

    public bool Bool() => (NextUInt() & 1U) != 0;
}

public static class Random
{
    [ThreadStatic]
    private static System.Random? _generator;

    private static System.Random Generator => _generator ??= new System.Random();

    public static void Seed(ulong seed)
    {
        _generator = new System.Random(unchecked((int)(seed ^ (seed >> 32))));
    }

    public static float Float01() => Generator.NextSingle();
    public static float Range(float minimum, float maximum) => minimum + (maximum - minimum) * Float01();

    public static int Range(int minimumInclusive, int maximumInclusive)
    {
        if (minimumInclusive > maximumInclusive)
        {
            throw new ArgumentOutOfRangeException(nameof(minimumInclusive));
        }

        return (int)Generator.NextInt64(minimumInclusive, (long)maximumInclusive + 1L);
    }

    public static bool Bool() => Generator.Next(2) != 0;
}
