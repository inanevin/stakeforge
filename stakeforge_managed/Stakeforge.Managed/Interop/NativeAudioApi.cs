using System.Runtime.InteropServices;

namespace SFG;

[StructLayout(LayoutKind.Sequential)]
internal unsafe struct NativeAudioApi
{
    internal uint Size;
    internal uint Version;
    internal delegate* unmanaged[Cdecl]<nint, uint, byte> Play;
    internal delegate* unmanaged[Cdecl]<nint, uint, byte> Pause;
    internal delegate* unmanaged[Cdecl]<nint, uint, byte> Stop;
    internal delegate* unmanaged[Cdecl]<nint, void> PauseAll;
    internal delegate* unmanaged[Cdecl]<nint, void> ResumeAll;
}
