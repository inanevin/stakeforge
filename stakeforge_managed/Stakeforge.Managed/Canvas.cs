using System;
using System.Runtime.InteropServices;

namespace SFG;

public enum CanvasAxisMode : byte
{
    Fixed,
    ParentRelative,
    Fill,
    SumChildren,
    MaxChildren,
    CopyOther,
}

public enum CanvasPositionMode : byte
{
    Flow,
    OffsetInParent,
    RelativeInParent,
    AbsoluteScreen,
}

public enum CanvasAnchor : byte
{
    Start,
    Center,
    End,
}

public enum CanvasFlow : byte
{
    None,
    Row,
    Column,
}

public enum CanvasClipMode : byte
{
    None,
    CpuRect,
    ScissorRect,
}

[Flags]
public enum CanvasWidgetFlags : ushort
{
    None = 0,
    Visible = 1 << 0,
    Overlay = 1 << 2,
    Focusable = 1 << 3,
    Input = 1 << 4,
    ScrollX = 1 << 5,
    ScrollY = 1 << 6,
    Disabled = 1 << 8,
}

public enum CanvasGradient : byte
{
    None,
    Horizontal,
    Vertical,
}

public enum CanvasTextRasterMode : byte
{
    Lcd,
    Grayscale,
    Sdf,
}

public enum CanvasEventType : byte
{
    Press,
    Release,
    Click,
    DoubleClick,
    HoverEnter,
    HoverExit,
    HoverMove,
    DragBegin,
    Drag,
    DragEnd,
    FocusGain,
    FocusLose,
    Key,
    Wheel,
}

[StructLayout(LayoutKind.Explicit, Size = 56)]
public struct CanvasLayout
{
    [FieldOffset(0)] public Vector2 SizeValue;
    [FieldOffset(8)] public Vector2 PositionValue;
    [FieldOffset(16)] public Vector4 ChildMargins;
    [FieldOffset(32)] public Vector2 ScrollOffset;
    [FieldOffset(40)] public float ChildSpacing;
    [FieldOffset(44)] public CanvasAxisMode SizeModeX;
    [FieldOffset(45)] public CanvasAxisMode SizeModeY;
    [FieldOffset(46)] public CanvasPositionMode PositionModeX;
    [FieldOffset(47)] public CanvasPositionMode PositionModeY;
    [FieldOffset(48)] public CanvasAnchor AnchorX;
    [FieldOffset(49)] public CanvasAnchor AnchorY;
    [FieldOffset(50)] public CanvasFlow Flow;
    [FieldOffset(52)] public CanvasWidgetFlags Flags;
    [FieldOffset(54)] public CanvasClipMode ChildClipMode;

    public static CanvasLayout Fixed(float width, float height)
    {
        return new CanvasLayout
        {
            SizeValue = new Vector2(width, height),
            SizeModeX = CanvasAxisMode.Fixed,
            SizeModeY = CanvasAxisMode.Fixed,
            Flags = CanvasWidgetFlags.Visible,
        };
    }

    public static CanvasLayout Fill()
    {
        return new CanvasLayout
        {
            SizeModeX = CanvasAxisMode.Fill,
            SizeModeY = CanvasAxisMode.Fill,
            Flags = CanvasWidgetFlags.Visible,
        };
    }
}

[StructLayout(LayoutKind.Explicit, Size = 64)]
public struct CanvasFrameStyle
{
    [FieldOffset(0)] public Color FillColorA;
    [FieldOffset(16)] public Color FillColorB;
    [FieldOffset(32)] public Color OutlineColor;
    [FieldOffset(48)] public float Rounding;
    [FieldOffset(52)] public float OutlineThickness;
    [FieldOffset(56)] public float AntiAliasThickness;
    [FieldOffset(60)] public ushort RoundingSegments;
    [FieldOffset(62)] public CanvasGradient Gradient;
    [FieldOffset(63)] public bool Filled;

    public CanvasFrameStyle(Color color)
    {
        this = default;
        FillColorA = color;
        FillColorB = color;
        OutlineColor = Color.Black;
        Filled = true;
    }
}

[StructLayout(LayoutKind.Explicit, Size = 32)]
public struct CanvasTextStyle
{
    [FieldOffset(0)] public FontHandle Font;
    [FieldOffset(8)] public Color Color;
    [FieldOffset(24)] public float PointSize;
    [FieldOffset(28)] public byte Spacing;
    [FieldOffset(29)] public CanvasTextRasterMode RasterMode;
    [FieldOffset(30)] public bool FlipUv;

    public CanvasTextStyle(float pointSize, Color color)
    {
        this = default;
        Font = FontHandle.Invalid;
        Color = color;
        PointSize = pointSize;
        RasterMode = CanvasTextRasterMode.Grayscale;
    }
}

[StructLayout(LayoutKind.Sequential)]
public readonly struct CanvasWidget : IEquatable<CanvasWidget>
{
    public static CanvasWidget Invalid { get; } = new(Entity.Invalid, 0);

    public readonly Entity CanvasEntity;
    public readonly uint Handle;

    internal CanvasWidget(Entity canvasEntity, uint handle)
    {
        CanvasEntity = canvasEntity;
        Handle = handle;
    }

    public bool IsValid => CanvasEntity.IsValid && Handle != 0;

    public bool Equals(CanvasWidget other)
    {
        return CanvasEntity == other.CanvasEntity && Handle == other.Handle;
    }

    public override bool Equals(object? obj)
    {
        return obj is CanvasWidget other && Equals(other);
    }

    public override int GetHashCode()
    {
        return HashCode.Combine(CanvasEntity, Handle);
    }

    public static bool operator ==(CanvasWidget left, CanvasWidget right)
    {
        return left.Equals(right);
    }

    public static bool operator !=(CanvasWidget left, CanvasWidget right)
    {
        return !left.Equals(right);
    }
}

[StructLayout(LayoutKind.Explicit, Size = 36)]
public struct CanvasEvent
{
    [FieldOffset(0)] public Vector2 Position;
    [FieldOffset(8)] public Vector2 Delta;
    [FieldOffset(16)] public uint WidgetHandle;
    [FieldOffset(20)] public Entity CanvasEntity;
    [FieldOffset(24)] public float WheelDelta;
    [FieldOffset(28)] public ushort Key;
    [FieldOffset(30)] public ushort ScanCode;
    [FieldOffset(32)] public CanvasEventType Type;
    [FieldOffset(33)] public MouseButton Button;
    [FieldOffset(34)] public InputAction Action;
    [FieldOffset(35)] public bool FromNavigation;

    public readonly CanvasWidget Widget => new(CanvasEntity, WidgetHandle);
}

public static unsafe class Canvas
{
    public static CanvasWidget CreateFrame(World world, Entity canvasEntity, CanvasLayout layout, CanvasFrameStyle style, CanvasWidget parent = default)
    {
        uint handle = ManagedRuntime.GetApi()->Canvas->CreateFrame(world.GetNative(), canvasEntity.Id, parent.Handle, &layout, &style);
        return new CanvasWidget(canvasEntity, handle);
    }

    public static CanvasWidget CreateText(World world, Entity canvasEntity, CanvasLayout layout, string text, CanvasTextStyle style, CanvasWidget parent = default)
    {
        nint utf8 = Marshal.StringToCoTaskMemUTF8(text);

        try
        {
            uint handle = ManagedRuntime.GetApi()->Canvas->CreateText(world.GetNative(), canvasEntity.Id, parent.Handle, &layout, (byte*)utf8, &style);
            return new CanvasWidget(canvasEntity, handle);
        }
        finally
        {
            Marshal.FreeCoTaskMem(utf8);
        }
    }

    public static CanvasWidget CreateImage(World world, Entity canvasEntity, CanvasLayout layout, TextureHandle texture, Color tint, CanvasWidget parent = default)
    {
        uint handle = ManagedRuntime.GetApi()->Canvas->CreateImage(world.GetNative(), canvasEntity.Id, parent.Handle, &layout, texture.Id, &tint);
        return new CanvasWidget(canvasEntity, handle);
    }

    public static CanvasWidget CreateButton(World world, Entity canvasEntity, CanvasLayout layout, string text, CanvasFrameStyle frameStyle, CanvasTextStyle textStyle, Color hoverColor, Color pressColor, CanvasWidget parent = default)
    {
        nint utf8 = Marshal.StringToCoTaskMemUTF8(text);

        try
        {
            uint handle = ManagedRuntime.GetApi()->Canvas->CreateButton(world.GetNative(), canvasEntity.Id, parent.Handle, &layout, (byte*)utf8, &frameStyle, &textStyle, &hoverColor, &pressColor);
            return new CanvasWidget(canvasEntity, handle);
        }
        finally
        {
            Marshal.FreeCoTaskMem(utf8);
        }
    }

    public static bool Destroy(World world, CanvasWidget widget)
    {
        return ManagedRuntime.GetApi()->Canvas->DestroyWidget(world.GetNative(), widget.CanvasEntity.Id, widget.Handle) != 0;
    }

    public static bool Clear(World world, Entity canvasEntity)
    {
        return ManagedRuntime.GetApi()->Canvas->ClearWidgets(world.GetNative(), canvasEntity.Id) != 0;
    }

    public static bool SetLayout(World world, CanvasWidget widget, CanvasLayout layout)
    {
        return ManagedRuntime.GetApi()->Canvas->SetLayout(world.GetNative(), widget.CanvasEntity.Id, widget.Handle, &layout) != 0;
    }

    public static bool SetVisible(World world, CanvasWidget widget, bool visible)
    {
        return ManagedRuntime.GetApi()->Canvas->SetVisible(world.GetNative(), widget.CanvasEntity.Id, widget.Handle, visible ? (byte)1 : (byte)0) != 0;
    }

    public static bool SetEnabled(World world, CanvasWidget widget, bool enabled)
    {
        return ManagedRuntime.GetApi()->Canvas->SetEnabled(world.GetNative(), widget.CanvasEntity.Id, widget.Handle, enabled ? (byte)1 : (byte)0) != 0;
    }

    public static bool SetText(World world, CanvasWidget widget, string text)
    {
        nint utf8 = Marshal.StringToCoTaskMemUTF8(text);

        try
        {
            return ManagedRuntime.GetApi()->Canvas->SetText(world.GetNative(), widget.CanvasEntity.Id, widget.Handle, (byte*)utf8) != 0;
        }
        finally
        {
            Marshal.FreeCoTaskMem(utf8);
        }
    }

    public static bool SetFrameStyle(World world, CanvasWidget widget, CanvasFrameStyle style)
    {
        return ManagedRuntime.GetApi()->Canvas->SetFrameStyle(world.GetNative(), widget.CanvasEntity.Id, widget.Handle, &style) != 0;
    }

    public static bool SetTextStyle(World world, CanvasWidget widget, CanvasTextStyle style)
    {
        return ManagedRuntime.GetApi()->Canvas->SetTextStyle(world.GetNative(), widget.CanvasEntity.Id, widget.Handle, &style) != 0;
    }

    public static bool SetImage(World world, CanvasWidget widget, TextureHandle texture, Color tint)
    {
        return ManagedRuntime.GetApi()->Canvas->SetImage(world.GetNative(), widget.CanvasEntity.Id, widget.Handle, texture.Id, &tint) != 0;
    }
}

