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

public static unsafe class Game
{
    public static bool TryGetRenderResolution(out RenderResolution resolution)
    {
        resolution = default;

        fixed (RenderResolution* resolutionPointer = &resolution)
        {
            return ManagedRuntime.GetApi()->Game->GetRenderResolution(resolutionPointer) != 0;
        }
    }

    public static bool SetRenderResolution(ushort width, ushort height)
    {
        return ManagedRuntime.GetApi()->Game->SetRenderResolution(width, height) != 0;
    }

    public static bool SetRenderResolution(RenderResolution resolution)
    {
        return SetRenderResolution(resolution.Width, resolution.Height);
    }

    public static bool LoadWorld(ulong world)
    {
        return ManagedRuntime.GetApi()->Game->LoadWorld(world) != 0;
    }

    public static void Quit()
    {
        ManagedRuntime.GetApi()->Game->Quit();
    }
}
