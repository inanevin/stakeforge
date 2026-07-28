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
    private static ProjectAssemblyLoadContext? _context;
    private static Assembly? _assembly;

    internal static void Load(string assemblyPath)
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

            Type[] discoveredTypes = candidateAssembly.GetTypes();

            foreach (Type type in discoveredTypes)
            {
                ManagedRuntime.LogInfo($"discovered C# type: {type.FullName ?? type.Name}");
            }

            ProjectAssemblyLoadContext? previousContext = _context;
            ManagedRuntime.LogInfo($"loaded C# project assembly: {assemblyPath}");
            previousContext?.Unload();

            _context = candidateContext;
            _assembly = candidateAssembly;
        }
        catch
        {
            candidateContext.Unload();
            throw;
        }
    }

    internal static void Unload()
    {
        ProjectAssemblyLoadContext? context = _context;
        context?.Unload();

        _assembly = null;
        _context = null;
    }
}
