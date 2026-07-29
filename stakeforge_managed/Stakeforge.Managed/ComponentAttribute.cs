using System;

namespace SFG;

[AttributeUsage(AttributeTargets.Struct, Inherited = false)]
public sealed class ComponentAttribute : Attribute
{
    public ulong Id { get; }
    public uint NativeSize { get; } = uint.MaxValue;
    public bool IsEngineComponent { get; }

    public ComponentAttribute()
    {
    }

    public ComponentAttribute(ulong id)
    {
        Id = id;
    }

    public ComponentAttribute(ulong id, uint nativeSize)
    {
        Id = id;
        NativeSize = nativeSize;
    }

    public ComponentAttribute(ulong id, uint nativeSize, bool isEngineComponent)
    {
        Id = id;
        NativeSize = nativeSize;
        IsEngineComponent = isEngineComponent;
    }
}
