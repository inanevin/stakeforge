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

    public float TimeScale
    {
        get
        {
            NativeApi* api = ManagedRuntime.GetApi();
            return api->World->GetTimeScale(GetNative());
        }
        set
        {
            NativeApi* api = ManagedRuntime.GetApi();
            api->World->SetTimeScale(GetNative(), value);
        }
    }

    public float ElapsedTime
    {
        get
        {
            NativeApi* api = ManagedRuntime.GetApi();
            return api->World->GetElapsedTime(GetNative());
        }
    }

    public float RealElapsedTime
    {
        get
        {
            NativeApi* api = ManagedRuntime.GetApi();
            return api->World->GetRealElapsedTime(GetNative());
        }
    }

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

    public bool TryGetEntityPositionAbsolute(Entity entity, out Vector3 position)
    {
        NativeApi* api = ManagedRuntime.GetApi();
        position = default;

        fixed (Vector3* positionPointer = &position)
        {
            return api->World->GetEntityPosAbs(GetNative(), entity.Id, positionPointer) != 0;
        }
    }

    public bool TryGetEntityRotationAbsolute(Entity entity, out Quaternion rotation)
    {
        NativeApi* api = ManagedRuntime.GetApi();
        rotation = default;

        fixed (Quaternion* rotationPointer = &rotation)
        {
            return api->World->GetEntityRotAbs(GetNative(), entity.Id, rotationPointer) != 0;
        }
    }

    public bool TryGetEntityScaleAbsolute(Entity entity, out Vector3 scale)
    {
        NativeApi* api = ManagedRuntime.GetApi();
        scale = default;

        fixed (Vector3* scalePointer = &scale)
        {
            return api->World->GetEntityScaleAbs(GetNative(), entity.Id, scalePointer) != 0;
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
            return api->World->GetComponent(GetNative(), entity.Id, componentType.Id, componentPointer, ComponentType<T>.NativeSize) != 0;
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
            return api->World->AddComponent(GetNative(), entity.Id, componentType.Id, componentPointer, ComponentType<T>.NativeSize) != 0;
        }
    }

    public bool SetComponent<T>(Entity entity, ComponentType<T> componentType, in T component) where T : unmanaged
    {
        NativeApi* api = ManagedRuntime.GetApi();

        fixed (T* componentPointer = &component)
        {
            return api->World->SetComponent(GetNative(), entity.Id, componentType.Id, componentPointer, ComponentType<T>.NativeSize) != 0;
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

    public WorldQuery Query<T>() where T : unmanaged
    {
        return new WorldQuery(GetNative()).Require<T>();
    }

    public WorldQuery Query<T1, T2>()
        where T1 : unmanaged
        where T2 : unmanaged
    {
        return new WorldQuery(GetNative()).Require<T1>().Require<T2>();
    }

    public WorldQuery Query<T1, T2, T3>()
        where T1 : unmanaged
        where T2 : unmanaged
        where T3 : unmanaged
    {
        return new WorldQuery(GetNative()).Require<T1>().Require<T2>().Require<T3>();
    }

    public WorldQuery Query<T1, T2, T3, T4>()
        where T1 : unmanaged
        where T2 : unmanaged
        where T3 : unmanaged
        where T4 : unmanaged
    {
        return new WorldQuery(GetNative()).Require<T1>().Require<T2>().Require<T3>().Require<T4>();
    }

     public WorldQuery Query<T1, T2, T3, T4, T5>()
        where T1 : unmanaged
        where T2 : unmanaged
        where T3 : unmanaged
        where T4 : unmanaged
        where T5 : unmanaged
    {
        return new WorldQuery(GetNative()).Require<T1>().Require<T2>().Require<T3>().Require<T4>().Require<T5>();
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

    public bool HideEntity(Entity entity)
    {
        NativeApi* api = ManagedRuntime.GetApi();
        return api->World->HideEntity(GetNative(), entity.Id) != 0;
    }

	public bool ShowEntity(Entity entity)
	{
		NativeApi* api = ManagedRuntime.GetApi();
		return api->World->ShowEntity(GetNative(), entity.Id) != 0;
	}

	/// <summary>
	/// Renders this entity's geometry in the first-person foreground layer.
	/// Foreground geometry keeps its own depth relationships but is never
	/// occluded by world geometry.
	/// </summary>
	public bool SetViewModel(Entity entity, bool enabled)
	{
		NativeApi* api = ManagedRuntime.GetApi();
		return api->World->SetViewModel(GetNative(), entity.Id, enabled ? (byte)1 : (byte)0) != 0;
	}

    public Entity FindEntityByGuid(EntityGuid guid)
    {
        NativeApi* api = ManagedRuntime.GetApi();
        return new Entity(api->World->FindEntityByGuid(GetNative(), guid.Id));
    }

    public Entity SpawnPrefab(PrefabHandle prefab)
    {
        return SpawnPrefab(prefab, Entity.Invalid, Vector3.Zero, Quaternion.Identity, Vector3.One);
    }

    public Entity SpawnPrefab(PrefabHandle prefab, Entity parent, Vector3 localPosition, Quaternion localRotation, Vector3 localScale)
    {
        NativeApi* api = ManagedRuntime.GetApi();
        return new Entity(api->World->SpawnPrefab(GetNative(), prefab.Id, parent.Id, &localPosition, &localRotation, &localScale));
    }

    public void DebugDrawLine(Vector3 from, Vector3 to, Color color, float thicknessPixels = 2.0f, DebugDrawDepth depth = DebugDrawDepth.DepthTested)
    {
        ManagedRuntime.GetApi()->World->DebugDrawLine(GetNative(), &from, &to, &color, thicknessPixels, depth);
    }

    public void DebugDrawArrow(Vector3 from, Vector3 to, Color color, float headLength = 0.15f, float headRadius = 0.06f, float thicknessPixels = 2.0f, DebugDrawDepth depth = DebugDrawDepth.DepthTested)
    {
        ManagedRuntime.GetApi()->World->DebugDrawArrow(GetNative(), &from, &to, &color, headLength, headRadius, thicknessPixels, depth);
    }

    public void DebugDrawTriangle(Vector3 point0, Vector3 point1, Vector3 point2, Color color)
    {
        ManagedRuntime.GetApi()->World->DebugDrawTriangle(GetNative(), &point0, &point1, &point2, &color);
    }

    public void DebugDrawPolyline(ReadOnlySpan<Vector3> points, Color color, float thicknessPixels = 2.0f, DebugDrawDepth depth = DebugDrawDepth.DepthTested, bool closed = false)
    {
        fixed (Vector3* pointsPointer = points)
        {
            ManagedRuntime.GetApi()->World->DebugDrawPolyline(GetNative(), pointsPointer, (uint)points.Length, &color, thicknessPixels, depth, closed ? (byte)1 : (byte)0);
        }
    }

    public void DebugDrawAabb(Vector3 boundsMin, Vector3 boundsMax, Color color, float thicknessPixels = 2.0f, DebugDrawDepth depth = DebugDrawDepth.DepthTested)
    {
        ManagedRuntime.GetApi()->World->DebugDrawAabb(GetNative(), &boundsMin, &boundsMax, &color, thicknessPixels, depth);
    }

    public void DebugDrawBox(Matrix4x3 transform, Vector3 halfExtents, Color color, float thicknessPixels = 2.0f, DebugDrawDepth depth = DebugDrawDepth.DepthTested)
    {
        ManagedRuntime.GetApi()->World->DebugDrawBox(GetNative(), &transform, &halfExtents, &color, thicknessPixels, depth);
    }

    public void DebugDrawRectangle(Vector3 center, Vector3 right, Vector3 up, Vector2 size, Color color, float thicknessPixels = 2.0f, DebugDrawDepth depth = DebugDrawDepth.DepthTested)
    {
        ManagedRuntime.GetApi()->World->DebugDrawRectangle(GetNative(), &center, &right, &up, &size, &color, thicknessPixels, depth);
    }

    public void DebugDrawArc(Vector3 center, Vector3 normal, Vector3 startDirection, float radius, float angleRadians, Color color, float thicknessPixels = 2.0f, DebugDrawDepth depth = DebugDrawDepth.DepthTested, uint segments = 16)
    {
        ManagedRuntime.GetApi()->World->DebugDrawArc(GetNative(), &center, &normal, &startDirection, radius, angleRadians, &color, thicknessPixels, depth, segments);
    }

    public void DebugDrawCircle(Vector3 center, float radius, Vector3 normal, Color color, float thicknessPixels = 2.0f, DebugDrawDepth depth = DebugDrawDepth.DepthTested, uint segments = 32)
    {
        ManagedRuntime.GetApi()->World->DebugDrawCircle(GetNative(), &center, radius, &normal, &color, thicknessPixels, depth, segments);
    }

    public void DebugDrawSphere(Vector3 center, float radius, Color color, float thicknessPixels = 2.0f, DebugDrawDepth depth = DebugDrawDepth.DepthTested, uint segments = 32)
    {
        ManagedRuntime.GetApi()->World->DebugDrawSphere(GetNative(), &center, radius, &color, thicknessPixels, depth, segments);
    }

    public void DebugDrawCapsule(Vector3 center, float radius, float halfHeight, Vector3 direction, Color color, float thicknessPixels = 2.0f, DebugDrawDepth depth = DebugDrawDepth.DepthTested, uint segments = 32)
    {
        ManagedRuntime.GetApi()->World->DebugDrawCapsule(GetNative(), &center, radius, halfHeight, &direction, &color, thicknessPixels, depth, segments);
    }

    public void DebugDrawCylinder(Vector3 center, float radius, float halfHeight, Vector3 direction, Color color, float thicknessPixels = 2.0f, DebugDrawDepth depth = DebugDrawDepth.DepthTested, uint segments = 32)
    {
        ManagedRuntime.GetApi()->World->DebugDrawCylinder(GetNative(), &center, radius, halfHeight, &direction, &color, thicknessPixels, depth, segments);
    }

    public void DebugDrawCone(Vector3 origin, Vector3 direction, float length, float halfAngleRadians, Color color, float thicknessPixels = 2.0f, DebugDrawDepth depth = DebugDrawDepth.DepthTested, uint segments = 24)
    {
        ManagedRuntime.GetApi()->World->DebugDrawCone(GetNative(), &origin, &direction, length, halfAngleRadians, &color, thicknessPixels, depth, segments);
    }

    public void DebugDrawText2D(Vector2 position, string text, Color color, float sizePixels = 14.0f, DebugDrawTextAlignment alignment = DebugDrawTextAlignment.TopLeft)
    {
        DebugDrawText2D(position, text, color, FontHandle.Invalid, sizePixels, alignment);
    }

    public void DebugDrawText2D(Vector2 position, string text, Color color, FontHandle font, float sizePixels = 14.0f, DebugDrawTextAlignment alignment = DebugDrawTextAlignment.TopLeft)
    {
        int byteCount = Encoding.UTF8.GetByteCount(text);
        Span<byte> utf8Text = byteCount + 1 <= 256 ? stackalloc byte[byteCount + 1] : new byte[byteCount + 1];
        Encoding.UTF8.GetBytes(text, utf8Text);
        utf8Text[byteCount] = 0;

        fixed (byte* textPointer = utf8Text)
        {
            ManagedRuntime.GetApi()->World->DebugDrawText2D(GetNative(), &position, textPointer, &color, sizePixels, alignment, font.Id);
        }
    }

    public void DebugDrawText3D(Vector3 position, string text, Color color, float sizePixels = 14.0f, DebugDrawDepth depth = DebugDrawDepth.AlwaysVisible, DebugDrawTextAlignment alignment = DebugDrawTextAlignment.Center, Vector2 screenOffset = default)
    {
        DebugDrawText3D(position, text, color, FontHandle.Invalid, sizePixels, depth, alignment, screenOffset);
    }

    public void DebugDrawText3D(Vector3 position, string text, Color color, FontHandle font, float sizePixels = 14.0f, DebugDrawDepth depth = DebugDrawDepth.AlwaysVisible, DebugDrawTextAlignment alignment = DebugDrawTextAlignment.Center, Vector2 screenOffset = default)
    {
        int byteCount = Encoding.UTF8.GetByteCount(text);
        Span<byte> utf8Text = byteCount + 1 <= 256 ? stackalloc byte[byteCount + 1] : new byte[byteCount + 1];
        Encoding.UTF8.GetBytes(text, utf8Text);
        utf8Text[byteCount] = 0;

        fixed (byte* textPointer = utf8Text)
        {
            ManagedRuntime.GetApi()->World->DebugDrawText3D(GetNative(), &position, textPointer, &color, sizePixels, depth, alignment, &screenOffset, font.Id);
        }
    }

    public void DebugDrawTexture3D(Vector3 position, TextureHandle texture, Vector2 sizePixels, Color color, DebugDrawDepth depth = DebugDrawDepth.AlwaysVisible, Vector2 screenOffset = default, bool linearSample = true)
    {
        DebugDrawTexture3D(position, texture, sizePixels, color, Entity.Invalid, depth, screenOffset, linearSample);
    }

    public void DebugDrawTexture3D(Vector3 position, TextureHandle texture, Vector2 sizePixels, Color color, Entity entity, DebugDrawDepth depth = DebugDrawDepth.AlwaysVisible, Vector2 screenOffset = default, bool linearSample = true)
    {
        ManagedRuntime.GetApi()->World->DebugDrawTexture3D(GetNative(), &position, texture.Id, &sizePixels, &color, entity.Id, depth, &screenOffset, linearSample ? (byte)1 : (byte)0);
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
