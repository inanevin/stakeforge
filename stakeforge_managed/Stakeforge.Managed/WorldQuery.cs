using System;

namespace SFG;

public unsafe struct WorldQuery
{
    private const int MaxComponents = 16;

    private NativeWorldQueryComponentArray _components;
    private readonly nint _world;
    private int _componentCount;

    internal WorldQuery(nint world)
    {
        _components = default;
        _world = world;
        _componentCount = 0;
    }

    public WorldQuery Require<T>() where T : unmanaged
    {
        WorldQuery query = this;
        query.Add<T>(NativeWorldQueryComponentFlags.Required);
        return query;
    }

    public WorldQuery Optional<T>() where T : unmanaged
    {
        WorldQuery query = this;
        query.Add<T>(NativeWorldQueryComponentFlags.Optional);
        return query;
    }

    public WorldQuery Exclude<T>() where T : unmanaged
    {
        WorldQuery query = this;
        query.Add<T>(NativeWorldQueryComponentFlags.Excluded);
        return query;
    }

    public WorldQueryEnumerator GetEnumerator()
    {
        return new WorldQueryEnumerator(_world, _components, _componentCount);
    }

    private void Add<T>(NativeWorldQueryComponentFlags flags) where T : unmanaged
    {
        ComponentType<T> componentType = ComponentType<T>.Value;

        if (!componentType.IsValid)
        {
            throw new InvalidOperationException($"{typeof(T).FullName} is not a component type.");
        }

        if (_componentCount == MaxComponents)
        {
            throw new InvalidOperationException($"A world query cannot contain more than {MaxComponents} component types.");
        }

        for (int componentIndex = 0; componentIndex < _componentCount; componentIndex++)
        {
            if (_components[componentIndex].TypeId == componentType.Id)
            {
                throw new InvalidOperationException($"{typeof(T).FullName} is already part of the world query.");
            }
        }

        _components[_componentCount] = new NativeWorldQueryComponent
        {
            TypeId = componentType.Id,
            Size = ComponentType<T>.NativeSize,
            Flags = flags,
        };
        _componentCount++;
    }
}

public unsafe ref struct WorldQueryEnumerator
{
    private NativeWorldQuery _query;
    private NativeWorldQueryRow _row;
    private bool _active;

    internal WorldQueryEnumerator(nint world, NativeWorldQueryComponentArray components, int componentCount)
    {
        _query = default;
        _row = default;
        _active = false;

        NativeApi* api = ManagedRuntime.GetApi();
        NativeWorldQuery query = default;
        NativeWorldQueryComponent* componentPointer = &components[0];

        if (api->World->QueryBegin(world, componentPointer, (uint)componentCount, &query) == 0)
        {
            throw new InvalidOperationException("The world query is invalid.");
        }

        _query = query;
        _active = true;
    }

    public WorldQueryRow Current
    {
        get
        {
            fixed (NativeWorldQueryRow* rowPointer = &_row)
            {
                return new WorldQueryRow(rowPointer);
            }
        }
    }

    public bool MoveNext()
    {
        if (!_active)
        {
            return false;
        }

        NativeApi* api = ManagedRuntime.GetApi();

        fixed (NativeWorldQuery* queryPointer = &_query)
        fixed (NativeWorldQueryRow* rowPointer = &_row)
        {
            if (api->World->QueryNext(queryPointer, rowPointer) != 0)
            {
                return true;
            }
        }

        _active = false;
        return false;
    }

    public void Dispose()
    {
        if (!_active)
        {
            return;
        }

        NativeApi* api = ManagedRuntime.GetApi();

        fixed (NativeWorldQuery* queryPointer = &_query)
        {
            api->World->QueryEnd(queryPointer);
        }

        _active = false;
    }
}

public readonly unsafe ref struct WorldQueryRow
{
    private readonly NativeWorldQueryRow* _row;

    internal WorldQueryRow(NativeWorldQueryRow* row)
    {
        _row = row;
    }

    public Entity Entity => new(_row->Entity);

    public ref T Get<T>() where T : unmanaged
    {
        T* component = Find<T>(out bool present);

        if (!present)
        {
            throw new InvalidOperationException($"The query row does not contain {typeof(T).FullName}.");
        }

        if (component == null)
        {
            throw new InvalidOperationException($"{typeof(T).FullName} has no component data.");
        }

        return ref *component;
    }

    public bool Has<T>() where T : unmanaged
    {
        Find<T>(out bool present);
        return present;
    }

    public OptionalComponentRef<T> GetOptional<T>() where T : unmanaged
    {
        T* component = Find<T>(out bool present);
        return new OptionalComponentRef<T>(component, present);
    }

    private T* Find<T>(out bool present) where T : unmanaged
    {
        ulong typeId = ComponentType<T>.Value.Id;

        for (int componentIndex = 0; componentIndex < _row->ComponentCount; componentIndex++)
        {
            if (_row->ComponentTypeIds[componentIndex] == typeId)
            {
                present = (_row->ComponentPresenceMask & (1u << componentIndex)) != 0;
                return (T*)_row->Components[componentIndex];
            }
        }

        present = false;
        return null;
    }
}

public readonly unsafe ref struct OptionalComponentRef<T> where T : unmanaged
{
    private readonly T* _component;
    private readonly bool _hasValue;

    internal OptionalComponentRef(T* component, bool hasValue)
    {
        _component = component;
        _hasValue = hasValue;
    }

    public bool HasValue => _hasValue;

    public ref T Value
    {
        get
        {
            if (!_hasValue)
            {
                throw new InvalidOperationException("The optional component is not present.");
            }

            if (_component == null)
            {
                throw new InvalidOperationException($"{typeof(T).FullName} has no component data.");
            }

            return ref *_component;
        }
    }
}
