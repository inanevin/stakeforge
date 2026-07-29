using System.Runtime.InteropServices;

namespace SFG;

public enum InputAction : byte
{
    Press,
    Release,
    Repeat,
}

public enum MouseButton : byte
{
    Left,
    Right,
    Middle,
    Extra1,
    Extra2,
    Unknown = byte.MaxValue,
}

[StructLayout(LayoutKind.Sequential)]
public struct KeyEvent
{
    public ushort Key;
    public ushort ScanCode;
    public InputAction Action;
}

[StructLayout(LayoutKind.Sequential)]
public struct MouseButtonEvent
{
    public Vector2 Position;
    public MouseButton Button;
    public InputAction Action;
}

[StructLayout(LayoutKind.Sequential)]
public struct MouseMoveEvent
{
    public Vector2 Position;
    public Vector2 Delta;
}

[StructLayout(LayoutKind.Sequential)]
public struct MouseWheelEvent
{
    public Vector2 Position;
    public float Delta;
}

[StructLayout(LayoutKind.Sequential)]
public struct PhysicsContactEvent
{
    public Vector3 Position;
    public Vector3 Normal;
    public Entity EntityA;
    public Entity EntityB;
    public Entity SubShapeA;
    public Entity SubShapeB;
    public float Penetration;
    public uint SubShapeIdA;
    public uint SubShapeIdB;
    internal byte ContactType;
    internal byte IsSensor;
    internal ushort Reserved;
}
