using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Runtime.Loader;
using SFG;

namespace SFG.ScriptHost;

internal sealed class ProjectAssemblyLoadContext : AssemblyLoadContext
{
    private readonly AssemblyDependencyResolver _resolver;

    internal ProjectAssemblyLoadContext(string assemblyPath)
        : base(isCollectible: true)
    {
        _resolver = new AssemblyDependencyResolver(assemblyPath);
    }

    protected override Assembly? Load(AssemblyName assemblyName)
    {
        Assembly managedAssembly = typeof(Entity).Assembly;

        if (assemblyName.Name == managedAssembly.GetName().Name)
        {
            return managedAssembly;
        }

        string? assemblyPath = _resolver.ResolveAssemblyToPath(assemblyName);

        if (assemblyPath is null)
        {
            return null;
        }

        using FileStream assemblyStream = File.Open(assemblyPath, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete);
        return LoadFromStream(assemblyStream);
    }

    protected override nint LoadUnmanagedDll(string unmanagedDllName)
    {
        string? libraryPath = _resolver.ResolveUnmanagedDllToPath(unmanagedDllName);

        if (libraryPath is null)
        {
            return 0;
        }

        return LoadUnmanagedDllFromPath(libraryPath);
    }
}

internal static class ProjectAssemblyRuntime
{
    private sealed class WorldScriptInstance
    {
        internal required WorldScript Script { get; init; }
        internal required World World { get; init; }
    }

    private static ProjectAssemblyLoadContext? _activeContext;
    private static Assembly? _activeAssembly;
    private static Dictionary<ulong, Type> _activeWorldScriptTypes = [];
    private static ProjectAssemblyLoadContext? _stagedContext;
    private static Assembly? _stagedAssembly;
    private static Dictionary<ulong, Type> _stagedWorldScriptTypes = [];
    private static string? _stagedSchema;
    private static int _activeWorldScriptInstanceCount;

    internal static void Stage(string assemblyPath)
    {
        ProjectAssemblyLoadContext candidateContext = new(assemblyPath);

        try
        {
            Assembly candidateAssembly;

            using (FileStream assemblyStream = File.Open(assemblyPath, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete))
            {
                string pdbPath = Path.ChangeExtension(assemblyPath, ".pdb");

                if (File.Exists(pdbPath))
                {
                    using FileStream pdbStream = File.Open(pdbPath, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete);
                    candidateAssembly = candidateContext.LoadFromStream(assemblyStream, pdbStream);
                }
                else
                {
                    candidateAssembly = candidateContext.LoadFromStream(assemblyStream);
                }
            }

            string candidateSchema = ProjectAssemblySchema.Discover(candidateAssembly, out Dictionary<ulong, Type> candidateWorldScriptTypes);
            DiscardStaged();

            _stagedContext = candidateContext;
            _stagedAssembly = candidateAssembly;
            _stagedWorldScriptTypes = candidateWorldScriptTypes;
            _stagedSchema = candidateSchema;
            ManagedRuntime.LogInfo($"staged C# project assembly: {assemblyPath}");
        }
        catch
        {
            candidateContext.Unload();
            throw;
        }
    }

    internal static string GetStagedSchema()
    {
        return _stagedSchema ?? throw new InvalidOperationException("No C# project assembly is staged.");
    }

    internal static void ActivateStaged()
    {
        if (_stagedContext is null || _stagedAssembly is null)
        {
            throw new InvalidOperationException("No C# project assembly is staged.");
        }

        if (_activeWorldScriptInstanceCount != 0)
        {
            throw new InvalidOperationException("C# world script instances must be destroyed before activating a staged project assembly.");
        }

        ProjectAssemblyLoadContext? previousContext = _activeContext;
        _activeWorldScriptTypes = [];
        _activeContext = _stagedContext;
        _activeAssembly = _stagedAssembly;
        _activeWorldScriptTypes = _stagedWorldScriptTypes;
        _stagedContext = null;
        _stagedAssembly = null;
        _stagedWorldScriptTypes = [];
        _stagedSchema = null;
        previousContext?.Unload();

        ManagedRuntime.LogInfo("activated the staged C# project assembly.");
    }

    internal static void DiscardStaged()
    {
        ProjectAssemblyLoadContext? context = _stagedContext;
        _stagedContext = null;
        _stagedAssembly = null;
        _stagedWorldScriptTypes = [];
        _stagedSchema = null;
        context?.Unload();
    }

    internal static void Unload()
    {
        if (_activeWorldScriptInstanceCount != 0)
        {
            throw new InvalidOperationException("C# world script instances must be destroyed before unloading the project assembly.");
        }

        DiscardStaged();

        ProjectAssemblyLoadContext? context = _activeContext;
        _activeWorldScriptTypes = [];
        context?.Unload();

        _activeAssembly = null;
        _activeContext = null;
    }

    internal static nint CreateWorldScript(ulong typeId, nint nativeWorld)
    {
        if (!_activeWorldScriptTypes.TryGetValue(typeId, out Type? type))
        {
            throw new InvalidOperationException($"C# world script type {typeId} is not loaded.");
        }

        WorldScript script = (WorldScript)(Activator.CreateInstance(type) ?? throw new InvalidOperationException($"Could not create C# world script {type.FullName}."));
        WorldScriptInstance instance = new()
        {
            Script = script,
            World = new World(nativeWorld),
        };
        GCHandle handle = GCHandle.Alloc(instance);
        _activeWorldScriptInstanceCount++;
        return GCHandle.ToIntPtr(handle);
    }

    internal static void DestroyWorldScript(nint instanceHandle)
    {
        GCHandle handle = GCHandle.FromIntPtr(instanceHandle);

        if (handle.Target is not WorldScriptInstance)
        {
            throw new InvalidOperationException("The C# world script instance handle is invalid.");
        }

        handle.Free();
        _activeWorldScriptInstanceCount--;
    }

    internal static void BeginPlayWorldScript(nint instanceHandle)
    {
        WorldScriptInstance instance = GetWorldScriptInstance(instanceHandle);
        instance.Script.BeginPlay(instance.World);
    }

    internal static void EndPlayWorldScript(nint instanceHandle)
    {
        WorldScriptInstance instance = GetWorldScriptInstance(instanceHandle);
        instance.Script.EndPlay(instance.World);
    }

    internal static void TickWorldScript(nint instanceHandle, float deltaTime)
    {
        WorldScriptInstance instance = GetWorldScriptInstance(instanceHandle);
        instance.Script.Tick(instance.World, deltaTime);
    }

    internal static void PostTickWorldScript(nint instanceHandle, float deltaTime)
    {
        WorldScriptInstance instance = GetWorldScriptInstance(instanceHandle);
        instance.Script.PostTick(instance.World, deltaTime);
    }

    internal static void PostPhysicsTickWorldScript(nint instanceHandle, float deltaTime)
    {
        WorldScriptInstance instance = GetWorldScriptInstance(instanceHandle);
        instance.Script.PostPhysicsTick(instance.World, deltaTime);
    }

    internal static void PostAnimationTickWorldScript(nint instanceHandle, float deltaTime)
    {
        WorldScriptInstance instance = GetWorldScriptInstance(instanceHandle);
        instance.Script.PostAnimationTick(instance.World, deltaTime);
    }

    internal static void DrawDebugWorldScript(nint instanceHandle)
    {
        WorldScriptInstance instance = GetWorldScriptInstance(instanceHandle);
        instance.Script.DrawDebug(instance.World);
    }

    internal static void KeyEventWorldScript(nint instanceHandle, ushort key, ushort scanCode, InputAction action)
    {
        WorldScriptInstance instance = GetWorldScriptInstance(instanceHandle);
        instance.Script.OnKeyEvent(instance.World, new KeyEvent
        {
            Key = key,
            ScanCode = scanCode,
            Action = action,
        });
    }

    internal static void MouseButtonEventWorldScript(nint instanceHandle, MouseButton button, InputAction action, float positionX, float positionY)
    {
        WorldScriptInstance instance = GetWorldScriptInstance(instanceHandle);
        instance.Script.OnMouseButtonEvent(instance.World, new MouseButtonEvent
        {
            Position = new Vector2(positionX, positionY),
            Button = button,
            Action = action,
        });
    }

    internal static void MouseMoveEventWorldScript(nint instanceHandle, float positionX, float positionY, float deltaX, float deltaY)
    {
        WorldScriptInstance instance = GetWorldScriptInstance(instanceHandle);
        instance.Script.OnMouseMoveEvent(instance.World, new MouseMoveEvent
        {
            Position = new Vector2(positionX, positionY),
            Delta = new Vector2(deltaX, deltaY),
        });
    }

    internal static void MouseWheelEventWorldScript(nint instanceHandle, float positionX, float positionY, float delta)
    {
        WorldScriptInstance instance = GetWorldScriptInstance(instanceHandle);
        instance.Script.OnMouseWheelEvent(instance.World, new MouseWheelEvent
        {
            Position = new Vector2(positionX, positionY),
            Delta = delta,
        });
    }

    internal static unsafe void PhysicsContactWorldScript(nint instanceHandle, PhysicsContactEvent* contact, byte contactType, byte isSensor)
    {
        WorldScriptInstance instance = GetWorldScriptInstance(instanceHandle);

        if (isSensor != 0)
        {
            switch (contactType)
            {
                case 0:
                    instance.Script.OnTriggerEnter(instance.World, *contact);
                    break;
                case 1:
                    instance.Script.OnTriggerStay(instance.World, *contact);
                    break;
                case 2:
                    instance.Script.OnTriggerExit(instance.World, *contact);
                    break;
            }

            return;
        }

        switch (contactType)
        {
            case 0:
                instance.Script.OnCollisionEnter(instance.World, *contact);
                break;
            case 1:
                instance.Script.OnCollisionStay(instance.World, *contact);
                break;
            case 2:
                instance.Script.OnCollisionExit(instance.World, *contact);
                break;
        }
    }

    private static WorldScriptInstance GetWorldScriptInstance(nint instanceHandle)
    {
        GCHandle handle = GCHandle.FromIntPtr(instanceHandle);
        return handle.Target as WorldScriptInstance ?? throw new InvalidOperationException("The C# world script instance handle is invalid.");
    }
}
