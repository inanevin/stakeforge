using System;
using System.Text;

namespace SFG;

public readonly unsafe struct World : IEquatable<World>
{
    private readonly nint _native;

    internal World(nint native)
    {
        _native = native;
    }

    public bool IsValid => _native != 0;

    public Entity CreateEntity(string? name = null)
    {
        NativeApi* api = ManagedRuntime.GetApi();

        if (name == null)
        {
            return new Entity(api->World->CreateEntity(GetNative(), null));
        }

        int byteCount = Encoding.UTF8.GetByteCount(name);
        Span<byte> utf8Name = byteCount + 1 <= 256 ? stackalloc byte[byteCount + 1] : new byte[byteCount + 1];
        Encoding.UTF8.GetBytes(name, utf8Name);
        utf8Name[byteCount] = 0;

        fixed (byte* namePointer = utf8Name)
        {
            return new Entity(api->World->CreateEntity(GetNative(), namePointer));
        }
    }

    public bool DestroyEntity(Entity entity)
    {
        NativeApi* api = ManagedRuntime.GetApi();
        return api->World->DestroyEntity(GetNative(), entity.Id) != 0;
    }

    public Entity DuplicateEntity(Entity entity)
    {
        NativeApi* api = ManagedRuntime.GetApi();
        return new Entity(api->World->DuplicateEntity(GetNative(), entity.Id));
    }

    public bool AttachEntity(Entity entity, Entity parent)
    {
        NativeApi* api = ManagedRuntime.GetApi();
        return api->World->AttachEntity(GetNative(), entity.Id, parent.Id) != 0;
    }

    public bool DetachEntity(Entity entity)
    {
        NativeApi* api = ManagedRuntime.GetApi();
        return api->World->DetachEntity(GetNative(), entity.Id) != 0;
    }

    public bool SetEntityPositionLocal(Entity entity, Vector3 position)
    {
        NativeApi* api = ManagedRuntime.GetApi();
        return api->World->SetEntityPosLocal(GetNative(), entity.Id, &position) != 0;
    }

    public bool SetEntityRotationLocal(Entity entity, Quaternion rotation)
    {
        NativeApi* api = ManagedRuntime.GetApi();
        return api->World->SetEntityRotLocal(GetNative(), entity.Id, &rotation) != 0;
    }

    public bool SetEntityScaleLocal(Entity entity, Vector3 scale)
    {
        NativeApi* api = ManagedRuntime.GetApi();
        return api->World->SetEntityScaleLocal(GetNative(), entity.Id, &scale) != 0;
    }

    public bool TeleportEntity(Entity entity, Vector3 position, Quaternion rotation, Vector3 scale)
    {
        NativeApi* api = ManagedRuntime.GetApi();
        return api->World->TeleportEntity(GetNative(), entity.Id, &position, &rotation, &scale) != 0;
    }

    public bool MarkEntityTeleported(Entity entity)
    {
        NativeApi* api = ManagedRuntime.GetApi();
        return api->World->MarkEntityTeleported(GetNative(), entity.Id) != 0;
    }

    public bool TryGetEntityPositionLocal(Entity entity, out Vector3 position)
    {
        NativeApi* api = ManagedRuntime.GetApi();
        position = default;

        fixed (Vector3* positionPointer = &position)
        {
            return api->World->GetEntityPosLocal(GetNative(), entity.Id, positionPointer) != 0;
        }
    }

    public bool TryGetEntityRotationLocal(Entity entity, out Quaternion rotation)
    {
        NativeApi* api = ManagedRuntime.GetApi();
        rotation = default;

        fixed (Quaternion* rotationPointer = &rotation)
        {
            return api->World->GetEntityRotLocal(GetNative(), entity.Id, rotationPointer) != 0;
        }
    }

    public bool TryGetEntityScaleLocal(Entity entity, out Vector3 scale)
    {
        NativeApi* api = ManagedRuntime.GetApi();
        scale = default;

        fixed (Vector3* scalePointer = &scale)
        {
            return api->World->GetEntityScaleLocal(GetNative(), entity.Id, scalePointer) != 0;
        }
    }

    public bool TryGetEntityPositionLastAbsolute(Entity entity, out Vector3 position)
    {
        NativeApi* api = ManagedRuntime.GetApi();
        position = default;

        fixed (Vector3* positionPointer = &position)
        {
            return api->World->GetEntityPosLastAbs(GetNative(), entity.Id, positionPointer) != 0;
        }
    }

    public bool TryGetEntityRotationLastAbsolute(Entity entity, out Quaternion rotation)
    {
        NativeApi* api = ManagedRuntime.GetApi();
        rotation = default;

        fixed (Quaternion* rotationPointer = &rotation)
        {
            return api->World->GetEntityRotLastAbs(GetNative(), entity.Id, rotationPointer) != 0;
        }
    }

    public bool TryGetEntityScaleLastAbsolute(Entity entity, out Vector3 scale)
    {
        NativeApi* api = ManagedRuntime.GetApi();
        scale = default;

        fixed (Vector3* scalePointer = &scale)
        {
            return api->World->GetEntityScaleLastAbs(GetNative(), entity.Id, scalePointer) != 0;
        }
    }

    public bool TryConvertAbsolutePositionToLocal(Entity entity, Vector3 position, out Vector3 localPosition)
    {
        NativeApi* api = ManagedRuntime.GetApi();
        localPosition = default;

        fixed (Vector3* localPositionPointer = &localPosition)
        {
            return api->World->AbsPosToLocal(GetNative(), entity.Id, &position, localPositionPointer) != 0;
        }
    }

    public bool TryConvertAbsoluteRotationToLocal(Entity entity, Quaternion rotation, out Quaternion localRotation)
    {
        NativeApi* api = ManagedRuntime.GetApi();
        localRotation = default;

        fixed (Quaternion* localRotationPointer = &localRotation)
        {
            return api->World->AbsRotToLocal(GetNative(), entity.Id, &rotation, localRotationPointer) != 0;
        }
    }

    public bool TryConvertAbsoluteScaleToLocal(Entity entity, Vector3 scale, out Vector3 localScale)
    {
        NativeApi* api = ManagedRuntime.GetApi();
        localScale = default;

        fixed (Vector3* localScalePointer = &localScale)
        {
            return api->World->AbsScaleToLocal(GetNative(), entity.Id, &scale, localScalePointer) != 0;
        }
    }

    public Entity GetEntityWithName(string name)
    {
        NativeApi* api = ManagedRuntime.GetApi();
        int byteCount = Encoding.UTF8.GetByteCount(name);
        Span<byte> utf8Name = byteCount + 1 <= 256 ? stackalloc byte[byteCount + 1] : new byte[byteCount + 1];
        Encoding.UTF8.GetBytes(name, utf8Name);
        utf8Name[byteCount] = 0;

        fixed (byte* namePointer = utf8Name)
        {
            return new Entity(api->World->GetEntityWithName(GetNative(), namePointer));
        }
    }

    public int GetEntitiesWithName(string name, Span<Entity> entities)
    {
        NativeApi* api = ManagedRuntime.GetApi();
        int byteCount = Encoding.UTF8.GetByteCount(name);
        Span<byte> utf8Name = byteCount + 1 <= 256 ? stackalloc byte[byteCount + 1] : new byte[byteCount + 1];
        Encoding.UTF8.GetBytes(name, utf8Name);
        utf8Name[byteCount] = 0;

        fixed (byte* namePointer = utf8Name)
        fixed (Entity* entityPointer = entities)
        {
            return (int)api->World->GetAllEntitiesWithName(GetNative(), namePointer, entityPointer, (uint)entities.Length);
        }
    }

    public Entity GetEntityWithComponent<T>(ComponentType<T> componentType) where T : unmanaged
    {
        NativeApi* api = ManagedRuntime.GetApi();
        return new Entity(api->World->GetEntityWithComponent(GetNative(), componentType.Id));
    }

    public bool IsAlive(Entity entity)
    {
        NativeApi* api = ManagedRuntime.GetApi();
        return api->World->IsAlive(GetNative(), entity.Id) != 0;
    }

    public bool HasComponent<T>(Entity entity, ComponentType<T> componentType) where T : unmanaged
    {
        NativeApi* api = ManagedRuntime.GetApi();
        return api->World->HasComponent(GetNative(), entity.Id, componentType.Id) != 0;
    }

    public bool TryGetComponent<T>(Entity entity, ComponentType<T> componentType, out T component) where T : unmanaged
    {
        NativeApi* api = ManagedRuntime.GetApi();
        component = default;

        fixed (T* componentPointer = &component)
        {
            return api->World->GetComponent(GetNative(), entity.Id, componentType.Id, componentPointer, (uint)sizeof(T)) != 0;
        }
    }

    public T GetComponent<T>(Entity entity, ComponentType<T> componentType) where T : unmanaged
    {
        if (!TryGetComponent(entity, componentType, out T component))
        {
            throw new InvalidOperationException("The entity does not have the requested component.");
        }

        return component;
    }

    public bool AddComponent<T>(Entity entity, ComponentType<T> componentType, in T component) where T : unmanaged
    {
        NativeApi* api = ManagedRuntime.GetApi();

        fixed (T* componentPointer = &component)
        {
            return api->World->AddComponent(GetNative(), entity.Id, componentType.Id, componentPointer, (uint)sizeof(T)) != 0;
        }
    }

    public bool SetComponent<T>(Entity entity, ComponentType<T> componentType, in T component) where T : unmanaged
    {
        NativeApi* api = ManagedRuntime.GetApi();

        fixed (T* componentPointer = &component)
        {
            return api->World->SetComponent(GetNative(), entity.Id, componentType.Id, componentPointer, (uint)sizeof(T)) != 0;
        }
    }

    public bool RemoveComponent<T>(Entity entity, ComponentType<T> componentType) where T : unmanaged
    {
        NativeApi* api = ManagedRuntime.GetApi();
        return api->World->RemoveComponent(GetNative(), entity.Id, componentType.Id) != 0;
    }

    public int GetEntitiesWithComponent<T>(ComponentType<T> componentType, Span<Entity> entities) where T : unmanaged
    {
        NativeApi* api = ManagedRuntime.GetApi();

        fixed (Entity* entityPointer = entities)
        {
            return (int)api->World->GetAllEntitiesWithComponent(GetNative(), componentType.Id, entityPointer, (uint)entities.Length);
        }
    }

    public Entity GetEntityWithTag(ulong tag)
    {
        NativeApi* api = ManagedRuntime.GetApi();
        return new Entity(api->World->GetEntityWithTag(GetNative(), tag));
    }

    public int GetEntitiesWithTag(ulong tag, Span<Entity> entities)
    {
        NativeApi* api = ManagedRuntime.GetApi();

        fixed (Entity* entityPointer = entities)
        {
            return (int)api->World->GetAllEntitiesWithTag(GetNative(), tag, entityPointer, (uint)entities.Length);
        }
    }

    public bool SetEntityTag(Entity entity, ulong tag, bool enabled)
    {
        NativeApi* api = ManagedRuntime.GetApi();
        return api->World->SetEntityTag(GetNative(), entity.Id, tag, enabled ? (byte)1 : (byte)0) != 0;
    }

    public bool Equals(World other)
    {
        return _native == other._native;
    }

    public override bool Equals(object? obj)
    {
        return obj is World other && Equals(other);
    }

    public override int GetHashCode()
    {
        return _native.GetHashCode();
    }

    public static bool operator ==(World left, World right)
    {
        return left.Equals(right);
    }

    public static bool operator !=(World left, World right)
    {
        return !left.Equals(right);
    }

    internal nint GetNative()
    {
        if (_native == 0)
        {
            throw new InvalidOperationException("The world is not valid.");
        }

        return _native;
    }
}
