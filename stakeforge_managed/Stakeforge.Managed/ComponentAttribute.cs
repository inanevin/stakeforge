using System;

namespace SFG;

[AttributeUsage(AttributeTargets.Struct, Inherited = false)]
public sealed class ComponentAttribute : Attribute
{
    public ulong Id { get; }

    public ComponentAttribute()
    {
    }

    public ComponentAttribute(ulong id)
    {
        Id = id;
    }
}
