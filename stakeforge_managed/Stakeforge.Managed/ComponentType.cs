namespace SFG;

public readonly struct ComponentType<T> where T : unmanaged
{
    public static ComponentType<T> Invalid { get; } = new(ulong.MaxValue);

    internal ulong Id { get; }

    internal ComponentType(ulong id)
    {
        Id = id;
    }

    public bool IsValid => Id != 0 && Id != ulong.MaxValue;
}
