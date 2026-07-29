using System;
using System.IO;
using System.Reflection;
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
    private static ProjectAssemblyLoadContext? _activeContext;
    private static Assembly? _activeAssembly;
    private static ProjectAssemblyLoadContext? _stagedContext;
    private static Assembly? _stagedAssembly;
    private static string? _stagedSchema;

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

            string candidateSchema = ProjectAssemblySchema.Discover(candidateAssembly);
            DiscardStaged();

            _stagedContext = candidateContext;
            _stagedAssembly = candidateAssembly;
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

        ProjectAssemblyLoadContext? previousContext = _activeContext;
        _activeContext = _stagedContext;
        _activeAssembly = _stagedAssembly;
        _stagedContext = null;
        _stagedAssembly = null;
        _stagedSchema = null;
        previousContext?.Unload();

        ManagedRuntime.LogInfo("activated the staged C# project assembly.");
    }

    internal static void DiscardStaged()
    {
        ProjectAssemblyLoadContext? context = _stagedContext;
        _stagedContext = null;
        _stagedAssembly = null;
        _stagedSchema = null;
        context?.Unload();
    }

    internal static void Unload()
    {
        DiscardStaged();

        ProjectAssemblyLoadContext? context = _activeContext;
        context?.Unload();

        _activeAssembly = null;
        _activeContext = null;
    }
}
