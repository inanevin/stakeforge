using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using SFG;

namespace SFG.ScriptHost;

public static unsafe class NativeEntryPoints
{
    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    public static int Initialize(void* api)
    {
        try
        {
            int result = ManagedRuntime.Initialize(api);

            if (result == 0)
            {
                ManagedRuntime.LogInfo("managed scripting host initialized correctly.");
            }

            return result;
        }
        catch (Exception exception)
        {
            ManagedRuntime.TryLogError(api, exception);
            ManagedRuntime.Shutdown();
            return -2;
        }
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    public static int Shutdown()
    {
        try
        {
            ProjectAssemblyRuntime.Unload();
            ManagedRuntime.LogInfo("managed scripting host shut down correctly.");
            return 0;
        }
        catch (Exception exception)
        {
            ManagedRuntime.TryLogError(exception);
            return -2;
        }
        finally
        {
            ManagedRuntime.Shutdown();
        }
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    public static int StageProjectAssembly(byte* assemblyPath)
    {
        try
        {
            string? path = Marshal.PtrToStringUTF8((nint)assemblyPath);

            if (string.IsNullOrWhiteSpace(path))
            {
                ManagedRuntime.TryLogError("the C# project assembly path is empty.");
                return -1;
            }

            ProjectAssemblyRuntime.Stage(path);
            return 0;
        }
        catch (Exception exception)
        {
            ManagedRuntime.TryLogError(exception);
            return -2;
        }
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    public static int GetStagedProjectSchema(byte* buffer, uint capacity)
    {
        try
        {
            string schema = ProjectAssemblyRuntime.GetStagedSchema();
            int requiredSize = Encoding.UTF8.GetByteCount(schema) + 1;

            if (buffer == null || capacity < requiredSize)
            {
                return requiredSize;
            }

            Span<byte> output = new(buffer, requiredSize);
            int bytesWritten = Encoding.UTF8.GetBytes(schema, output);
            output[bytesWritten] = 0;
            return requiredSize;
        }
        catch (Exception exception)
        {
            ManagedRuntime.TryLogError(exception);
            return -2;
        }
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    public static int ActivateStagedProjectAssembly()
    {
        try
        {
            ProjectAssemblyRuntime.ActivateStaged();
            return 0;
        }
        catch (Exception exception)
        {
            ManagedRuntime.TryLogError(exception);
            return -2;
        }
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    public static int DiscardStagedProjectAssembly()
    {
        try
        {
            ProjectAssemblyRuntime.DiscardStaged();
            return 0;
        }
        catch (Exception exception)
        {
            ManagedRuntime.TryLogError(exception);
            return -2;
        }
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    public static nint CreateWorldScript(ulong typeId, nint nativeWorld)
    {
        try
        {
            return ProjectAssemblyRuntime.CreateWorldScript(typeId, nativeWorld);
        }
        catch (Exception exception)
        {
            ManagedRuntime.TryLogError(exception);
            return 0;
        }
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    public static int DestroyWorldScript(nint instance)
    {
        try
        {
            ProjectAssemblyRuntime.DestroyWorldScript(instance);
            return 0;
        }
        catch (Exception exception)
        {
            ManagedRuntime.TryLogError(exception);
            return -2;
        }
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    public static int BeginPlayWorldScript(nint instance)
    {
        try
        {
            ProjectAssemblyRuntime.BeginPlayWorldScript(instance);
            return 0;
        }
        catch (Exception exception)
        {
            ManagedRuntime.TryLogError(exception);
            return -2;
        }
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    public static int EndPlayWorldScript(nint instance)
    {
        try
        {
            ProjectAssemblyRuntime.EndPlayWorldScript(instance);
            return 0;
        }
        catch (Exception exception)
        {
            ManagedRuntime.TryLogError(exception);
            return -2;
        }
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    public static int TickWorldScript(nint instance, float deltaTime)
    {
        try
        {
            ProjectAssemblyRuntime.TickWorldScript(instance, deltaTime);
            return 0;
        }
        catch (Exception exception)
        {
            ManagedRuntime.TryLogError(exception);
            return -2;
        }
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    public static int PostTickWorldScript(nint instance, float deltaTime)
    {
        try
        {
            ProjectAssemblyRuntime.PostTickWorldScript(instance, deltaTime);
            return 0;
        }
        catch (Exception exception)
        {
            ManagedRuntime.TryLogError(exception);
            return -2;
        }
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    public static int PostPhysicsTickWorldScript(nint instance, float deltaTime)
    {
        try
        {
            ProjectAssemblyRuntime.PostPhysicsTickWorldScript(instance, deltaTime);
            return 0;
        }
        catch (Exception exception)
        {
            ManagedRuntime.TryLogError(exception);
            return -2;
        }
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    public static int PostAnimationTickWorldScript(nint instance, float deltaTime)
    {
        try
        {
            ProjectAssemblyRuntime.PostAnimationTickWorldScript(instance, deltaTime);
            return 0;
        }
        catch (Exception exception)
        {
            ManagedRuntime.TryLogError(exception);
            return -2;
        }
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    public static int KeyEventWorldScript(nint instance, ushort key, ushort scanCode, byte action)
    {
        try
        {
            ProjectAssemblyRuntime.KeyEventWorldScript(instance, key, scanCode, (InputAction)action);
            return 0;
        }
        catch (Exception exception)
        {
            ManagedRuntime.TryLogError(exception);
            return -2;
        }
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    public static int MouseButtonEventWorldScript(nint instance, byte button, byte action, float positionX, float positionY)
    {
        try
        {
            ProjectAssemblyRuntime.MouseButtonEventWorldScript(instance, (MouseButton)button, (InputAction)action, positionX, positionY);
            return 0;
        }
        catch (Exception exception)
        {
            ManagedRuntime.TryLogError(exception);
            return -2;
        }
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    public static int MouseMoveEventWorldScript(nint instance, float positionX, float positionY, float deltaX, float deltaY)
    {
        try
        {
            ProjectAssemblyRuntime.MouseMoveEventWorldScript(instance, positionX, positionY, deltaX, deltaY);
            return 0;
        }
        catch (Exception exception)
        {
            ManagedRuntime.TryLogError(exception);
            return -2;
        }
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    public static int MouseWheelEventWorldScript(nint instance, float positionX, float positionY, float delta)
    {
        try
        {
            ProjectAssemblyRuntime.MouseWheelEventWorldScript(instance, positionX, positionY, delta);
            return 0;
        }
        catch (Exception exception)
        {
            ManagedRuntime.TryLogError(exception);
            return -2;
        }
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    public static int PhysicsContactWorldScript(nint instance, PhysicsContactEvent* contact, byte contactType, byte isSensor)
    {
        try
        {
            ProjectAssemblyRuntime.PhysicsContactWorldScript(instance, contact, contactType, isSensor);
            return 0;
        }
        catch (Exception exception)
        {
            ManagedRuntime.TryLogError(exception);
            return -2;
        }
    }

}
