using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace SFG;

internal enum NativeWorldQueryComponentFlags : byte
{
    Required = 0,
    Excluded = 1,
    Optional = 2,
}

[StructLayout(LayoutKind.Sequential)]
internal struct NativeWorldQueryComponent
{
    internal ulong TypeId;
    internal uint Size;
    internal NativeWorldQueryComponentFlags Flags;
    internal byte Reserved0;
    internal byte Reserved1;
    internal byte Reserved2;
}

[InlineArray(16)]
internal struct NativeWorldQueryComponentArray
{
    private NativeWorldQueryComponent _element0;
}

[InlineArray(96)]
internal struct NativeWorldQueryStorage
{
    private ulong _element0;
}

[StructLayout(LayoutKind.Sequential)]
internal struct NativeWorldQuery
{
    internal NativeWorldQueryStorage Storage;
}

[InlineArray(16)]
internal struct NativeWorldQueryPointerArray
{
    private nint _element0;
}

[InlineArray(16)]
internal struct NativeWorldQueryTypeIdArray
{
    private ulong _element0;
}

[StructLayout(LayoutKind.Sequential)]
internal struct NativeWorldQueryRow
{
    internal NativeWorldQueryPointerArray Components;
    internal NativeWorldQueryTypeIdArray ComponentTypeIds;
    internal uint Entity;
    internal uint ComponentCount;
    internal uint ComponentPresenceMask;
    internal uint Reserved;
}
