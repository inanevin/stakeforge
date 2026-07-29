using System.Runtime.InteropServices;

namespace SFG;

[StructLayout(LayoutKind.Sequential)]
internal unsafe struct NativeCanvasApi
{
    internal uint Size;
    internal uint Version;
    internal delegate* unmanaged[Cdecl]<nint, uint, uint, CanvasLayout*, CanvasFrameStyle*, uint> CreateFrame;
    internal delegate* unmanaged[Cdecl]<nint, uint, uint, CanvasLayout*, byte*, CanvasTextStyle*, uint> CreateText;
    internal delegate* unmanaged[Cdecl]<nint, uint, uint, CanvasLayout*, ulong, Color*, uint> CreateImage;
    internal delegate* unmanaged[Cdecl]<nint, uint, uint, CanvasLayout*, byte*, CanvasFrameStyle*, CanvasTextStyle*, Color*, Color*, uint> CreateButton;
    internal delegate* unmanaged[Cdecl]<nint, uint, uint, byte> DestroyWidget;
    internal delegate* unmanaged[Cdecl]<nint, uint, byte> ClearWidgets;
    internal delegate* unmanaged[Cdecl]<nint, uint, uint, CanvasLayout*, byte> SetLayout;
    internal delegate* unmanaged[Cdecl]<nint, uint, uint, byte, byte> SetVisible;
    internal delegate* unmanaged[Cdecl]<nint, uint, uint, byte, byte> SetEnabled;
    internal delegate* unmanaged[Cdecl]<nint, uint, uint, byte*, byte> SetText;
    internal delegate* unmanaged[Cdecl]<nint, uint, uint, CanvasFrameStyle*, byte> SetFrameStyle;
    internal delegate* unmanaged[Cdecl]<nint, uint, uint, CanvasTextStyle*, byte> SetTextStyle;
    internal delegate* unmanaged[Cdecl]<nint, uint, uint, ulong, Color*, byte> SetImage;
}
