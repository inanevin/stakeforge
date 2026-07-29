using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text.Json;
using SFG;

namespace SFG.ScriptHost;

internal sealed class ProjectComponentFieldSchema
{
    public required string Name { get; init; }
    public required ulong Id { get; init; }
    public required string ValueType { get; init; }
    public required ulong SubTypeId { get; init; }
    public required uint Offset { get; init; }
    public required uint Size { get; init; }
    public required bool NoUi { get; init; }
}

internal sealed class ProjectComponentSchema
{
    public required string Name { get; init; }
    public required string FullName { get; init; }
    public required ulong Id { get; init; }
    public required uint Size { get; init; }
    public required uint Alignment { get; init; }
    public required List<ProjectComponentFieldSchema> Fields { get; init; }
}

internal sealed class ProjectWorldScriptSchema
{
    public required string Name { get; init; }
    public required string FullName { get; init; }
    public required ulong Id { get; init; }
}

internal sealed class ProjectAssemblySchema
{
    private static readonly MethodInfo SizeOfMethod = typeof(ProjectAssemblySchema).GetMethod(nameof(SizeOf), BindingFlags.NonPublic | BindingFlags.Static)!;
    private static readonly JsonSerializerOptions JsonOptions = new()
    {
        PropertyNamingPolicy = JsonNamingPolicy.SnakeCaseLower,
    };

    public required List<ProjectComponentSchema> Components { get; init; }
    public required List<ProjectWorldScriptSchema> WorldScripts { get; init; }

    internal static string Discover(Assembly assembly, out Dictionary<ulong, Type> worldScriptTypes)
    {
        List<ProjectComponentSchema> components = [];
        HashSet<ulong> componentIds = [];
        List<ProjectWorldScriptSchema> worldScripts = [];
        worldScriptTypes = [];

        foreach (Type type in assembly.GetTypes().OrderBy(type => type.FullName, StringComparer.Ordinal))
        {
            ComponentAttribute? attribute = type.GetCustomAttribute<ComponentAttribute>();

            if (attribute is null)
            {
                continue;
            }

            ProjectComponentSchema component = DiscoverComponent(type, attribute);

            if (!componentIds.Add(component.Id))
            {
                throw new InvalidOperationException($"C# component id {component.Id} is used more than once.");
            }

            components.Add(component);
            ManagedRuntime.LogInfo($"discovered C# component: {component.FullName} ({component.Id}).");
        }

        foreach (Type type in assembly.GetTypes().OrderBy(type => type.FullName, StringComparer.Ordinal))
        {
            if (!type.IsSubclassOf(typeof(WorldScript)) || type.IsAbstract)
            {
                continue;
            }

            if (!type.IsClass || type.IsGenericTypeDefinition || type.GetConstructor(Type.EmptyTypes) is null)
            {
                throw new InvalidOperationException($"C# world script {type.FullName} must be a concrete, non-generic class with a public parameterless constructor.");
            }

            string fullName = type.FullName ?? type.Name;
            ulong typeId = Hash.StringId(fullName);

            if (typeId == 0 || typeId == ulong.MaxValue || !worldScriptTypes.TryAdd(typeId, type))
            {
                throw new InvalidOperationException($"C# world script {fullName} has an invalid or duplicate type id.");
            }

            worldScripts.Add(new ProjectWorldScriptSchema
            {
                Name = type.Name,
                FullName = fullName,
                Id = typeId,
            });

            ManagedRuntime.LogInfo($"discovered C# world script: {fullName} ({typeId}).");
        }

        return JsonSerializer.Serialize(new ProjectAssemblySchema
        {
            Components = components,
            WorldScripts = worldScripts,
        }, JsonOptions);
    }

    private static ProjectComponentSchema DiscoverComponent(Type type, ComponentAttribute attribute)
    {
        if (!type.IsValueType || type.IsEnum)
        {
            throw new InvalidOperationException($"C# component {type.FullName} must be a struct.");
        }

        StructLayoutAttribute? layout = type.StructLayoutAttribute;

        if (layout is null || layout.Value != LayoutKind.Sequential)
        {
            throw new InvalidOperationException($"C# component {type.FullName} must use StructLayout(LayoutKind.Sequential).");
        }

        int managedSize = (int)SizeOfMethod.MakeGenericMethod(type).Invoke(null, null)!;
        int marshalledSize = Marshal.SizeOf(type);

        if (managedSize != marshalledSize)
        {
            throw new InvalidOperationException($"C# component {type.FullName} has a managed size of {managedSize} but an interop size of {marshalledSize}. Use only supported unmanaged fields. Bool fields require [MarshalAs(UnmanagedType.U1)].");
        }

        FieldInfo[] allFields = type.GetFields(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic);

        if (allFields.Any(field => field.IsDefined(typeof(CompilerGeneratedAttribute), false)))
        {
            throw new InvalidOperationException($"C# component {type.FullName} may not contain compiler-generated instance fields. Declare component data as explicit fields instead of auto-properties.");
        }

        List<ProjectComponentFieldSchema> fields = [];
        uint alignment = 1;

        foreach (FieldInfo field in allFields.OrderBy(field => Marshal.OffsetOf(type, field.Name).ToInt64()))
        {
            (string valueType, ulong subTypeId, uint fieldSize, uint fieldAlignment) = GetFieldLayout(field);
            long fieldOffset = Marshal.OffsetOf(type, field.Name).ToInt64();
            ComponentFieldAttribute? fieldAttribute = field.GetCustomAttribute<ComponentFieldAttribute>();
            ulong fieldId = fieldAttribute?.Id ?? Hash.StringId(field.Name);

            if (fieldOffset < 0 || fieldOffset > uint.MaxValue || fieldId == 0 || fieldId == ulong.MaxValue)
            {
                throw new InvalidOperationException($"C# component field {type.FullName}.{field.Name} has an invalid id or unsupported offset.");
            }

            fields.Add(new ProjectComponentFieldSchema
            {
                Name = field.Name,
                Id = fieldId,
                ValueType = valueType,
                SubTypeId = subTypeId,
                Offset = (uint)fieldOffset,
                Size = fieldSize,
                NoUi = !field.IsPublic,
            });

            alignment = System.Math.Max(alignment, fieldAlignment);
        }

        if (layout.Pack > 0)
        {
            alignment = System.Math.Min(alignment, (uint)layout.Pack);
        }

        string fullName = type.FullName ?? type.Name;
        ulong componentId = attribute.Id != 0 ? attribute.Id : Hash.StringId(fullName);

        if (componentId == 0 || componentId == ulong.MaxValue)
        {
            throw new InvalidOperationException($"C# component {fullName} has an invalid component id.");
        }

        return new ProjectComponentSchema
        {
            Name = type.Name,
            FullName = fullName,
            Id = componentId,
            Size = (uint)managedSize,
            Alignment = alignment,
            Fields = fields,
        };
    }

    private static (string ValueType, ulong SubTypeId, uint Size, uint Alignment) GetFieldLayout(FieldInfo field)
    {
        Type type = field.FieldType;

        if (type == typeof(bool))
        {
            MarshalAsAttribute? marshalAs = field.GetCustomAttribute<MarshalAsAttribute>();

            if (marshalAs?.Value != UnmanagedType.U1)
            {
                throw new InvalidOperationException($"C# component bool field {field.DeclaringType?.FullName}.{field.Name} requires [MarshalAs(UnmanagedType.U1)].");
            }

            return ("boolean", 0, 1, 1);
        }

        if (type == typeof(float)) return ("f32", 0, 4, 4);
        if (type == typeof(ulong)) return ("u64", 0, 8, 8);
        if (type == typeof(long)) return ("i64", 0, 8, 8);
        if (type == typeof(uint)) return ("u32", 0, 4, 4);
        if (type == typeof(int)) return ("i32", 0, 4, 4);
        if (type == typeof(ushort)) return ("u16", 0, 2, 2);
        if (type == typeof(short)) return ("i16", 0, 2, 2);
        if (type == typeof(byte)) return ("u8", 0, 1, 1);
        if (type == typeof(sbyte)) return ("i8", 0, 1, 1);
        if (type == typeof(EntityGuid)) return ("u64", Hash.StringId("reflection_subtype_entity_guid"), 8, 8);
        if (type == typeof(AudioHandle)) return ("u64", Hash.StringId("reflection_resource_subtype_audio"), 8, 8);
        if (type == typeof(FontHandle)) return ("u64", Hash.StringId("reflection_resource_subtype_font"), 8, 8);
        if (type == typeof(MeshHandle)) return ("u64", Hash.StringId("reflection_resource_subtype_mesh"), 8, 8);
        if (type == typeof(SkeletonHandle)) return ("u64", Hash.StringId("reflection_resource_subtype_skeleton"), 8, 8);
        if (type == typeof(AnimationHandle)) return ("u64", Hash.StringId("reflection_resource_subtype_animation"), 8, 8);
        if (type == typeof(MaterialHandle)) return ("u64", Hash.StringId("reflection_resource_subtype_material"), 8, 8);
        if (type == typeof(ShaderHandle)) return ("u64", Hash.StringId("reflection_resource_subtype_shader"), 8, 8);
        if (type == typeof(TextureHandle)) return ("u64", Hash.StringId("reflection_resource_subtype_texture"), 8, 8);
        if (type == typeof(TextureSamplerHandle)) return ("u64", Hash.StringId("reflection_resource_subtype_texture_sampler"), 8, 8);
        if (type == typeof(PhysicalMaterialHandle)) return ("u64", Hash.StringId("reflection_resource_subtype_physical_material"), 8, 8);
        if (type == typeof(PrefabHandle)) return ("u64", Hash.StringId("reflection_resource_subtype_prefab"), 8, 8);
        if (type == typeof(AnimationGraphHandle)) return ("u64", Hash.StringId("reflection_resource_subtype_animation_graph"), 8, 8);
        if (type == typeof(CubemapHandle)) return ("u64", Hash.StringId("reflection_resource_subtype_cubemap"), 8, 8);
        if (type == typeof(PhysicsCollisionMeshHandle)) return ("u64", Hash.StringId("reflection_resource_subtype_physics_collision_mesh"), 8, 8);
        if (type == typeof(SpriteHandle)) return ("u64", Hash.StringId("reflection_resource_subtype_sprite"), 8, 8);
        if (type == typeof(CurveHandle)) return ("u64", Hash.StringId("reflection_resource_subtype_curve"), 8, 8);
        if (type == typeof(Vector2)) return ("object", Hash.StringId("vec2f_t"), 8, 4);
        if (type == typeof(Vector3)) return ("object", Hash.StringId("vec3f_t"), 12, 4);
        if (type == typeof(Vector4)) return ("object", Hash.StringId("vec4f_t"), 16, 4);
        if (type == typeof(Quaternion)) return ("object", Hash.StringId("quat_t"), 16, 4);
        if (type == typeof(Matrix4x3)) return ("object", Hash.StringId("mat4x3_t"), 48, 4);
        if (type == typeof(Matrix4x4)) return ("object", Hash.StringId("mat4x4_t"), 64, 4);

        throw new InvalidOperationException($"C# component field {field.DeclaringType?.FullName}.{field.Name} has unsupported type {type.FullName}.");
    }

    private static int SizeOf<T>() where T : unmanaged
    {
        return Unsafe.SizeOf<T>();
    }
}
