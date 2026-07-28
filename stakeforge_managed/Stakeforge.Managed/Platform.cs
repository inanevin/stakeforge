namespace SFG;

public enum WindowStyle : byte
{
    AppWindow,
    Borderless,
    Alpha,
}

public static unsafe class Platform
{
    public static void SetCursorVisible(bool visible)
    {
        ManagedRuntime.GetApi()->Platform->SetCursorVisible(visible ? (byte)1 : (byte)0);
    }

    public static void LockCursor(bool locked)
    {
        ManagedRuntime.GetApi()->Platform->LockCursor(locked ? (byte)1 : (byte)0);
    }

    public static void SetWindowSize(ushort width, ushort height)
    {
        ManagedRuntime.GetApi()->Platform->SetWindowSize(width, height);
    }

    public static void SetWindowStyle(WindowStyle style)
    {
        ManagedRuntime.GetApi()->Platform->SetWindowStyle(style);
    }
}
