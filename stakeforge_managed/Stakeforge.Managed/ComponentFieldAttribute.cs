using System;

namespace SFG;

[AttributeUsage(AttributeTargets.Field, Inherited = false)]
public sealed class ComponentFieldAttribute : Attribute
{
    public ulong Id { get; }

    public ComponentFieldAttribute(ulong id)
    {
        Id = id;
    }
}
