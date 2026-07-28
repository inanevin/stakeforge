using System.Runtime.InteropServices;

namespace SFG;

[StructLayout(LayoutKind.Sequential)]
public readonly struct RenderResolution
{
    public readonly ushort Width;
    public readonly ushort Height;

    public RenderResolution(ushort width, ushort height)
    {
        Width = width;
        Height = height;
    }
}

public static unsafe class Render
{
    public static bool TryGetResolution(out RenderResolution resolution)
    {
        resolution = default;

        fixed (RenderResolution* resolutionPointer = &resolution)
        {
            return ManagedRuntime.GetApi()->Render->GetRenderResolution(resolutionPointer) != 0;
        }
    }

    public static bool SetResolution(ushort width, ushort height)
    {
        return ManagedRuntime.GetApi()->Render->SetRenderResolution(width, height) != 0;
    }

    public static bool SetResolution(RenderResolution resolution)
    {
        return SetResolution(resolution.Width, resolution.Height);
    }
}
