using System;

namespace SFG;

public readonly struct ComponentType<T> where T : unmanaged
{
    public static ComponentType<T> Invalid { get; } = new(ulong.MaxValue);
    public static ComponentType<T> Value { get; } = new(GetTypeId());

    public ulong Id { get; }

    internal ComponentType(ulong id)
    {
        Id = id;
    }

    public bool IsValid => Id != 0 && Id != ulong.MaxValue;

    private static ulong GetTypeId()
    {
        Type type = typeof(T);
        ComponentAttribute? attribute = Attribute.GetCustomAttribute(type, typeof(ComponentAttribute)) as ComponentAttribute;

        if (attribute is null)
        {
            return 0;
        }

        return attribute.Id != 0 ? attribute.Id : Hash.StringId(type.FullName ?? type.Name);
    }
}
