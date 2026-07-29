using System;
using System.Runtime.InteropServices;

namespace SFG;

[StructLayout(LayoutKind.Sequential)]
public readonly struct AudioHandle : IEquatable<AudioHandle>
{
    public static AudioHandle Invalid { get; } = new(ulong.MaxValue);
    public readonly ulong Id;
    public AudioHandle(ulong id) => Id = id;
    public bool IsValid => Id != ulong.MaxValue;
    public bool Equals(AudioHandle other) => Id == other.Id;
    public override bool Equals(object? obj) => obj is AudioHandle other && Equals(other);
    public override int GetHashCode() => Id.GetHashCode();
    public static bool operator ==(AudioHandle left, AudioHandle right) => left.Equals(right);
    public static bool operator !=(AudioHandle left, AudioHandle right) => !left.Equals(right);
    public static implicit operator ResourceHandle(AudioHandle value) => new(value.Id);
    public static explicit operator AudioHandle(ResourceHandle value) => new(value.Id);
}

[StructLayout(LayoutKind.Sequential)]
public readonly struct FontHandle : IEquatable<FontHandle>
{
    public static FontHandle Invalid { get; } = new(ulong.MaxValue);
    public readonly ulong Id;
    public FontHandle(ulong id) => Id = id;
    public bool IsValid => Id != ulong.MaxValue;
    public bool Equals(FontHandle other) => Id == other.Id;
    public override bool Equals(object? obj) => obj is FontHandle other && Equals(other);
    public override int GetHashCode() => Id.GetHashCode();
    public static bool operator ==(FontHandle left, FontHandle right) => left.Equals(right);
    public static bool operator !=(FontHandle left, FontHandle right) => !left.Equals(right);
    public static implicit operator ResourceHandle(FontHandle value) => new(value.Id);
    public static explicit operator FontHandle(ResourceHandle value) => new(value.Id);
}

[StructLayout(LayoutKind.Sequential)]
public readonly struct MeshHandle : IEquatable<MeshHandle>
{
    public static MeshHandle Invalid { get; } = new(ulong.MaxValue);
    public readonly ulong Id;
    public MeshHandle(ulong id) => Id = id;
    public bool IsValid => Id != ulong.MaxValue;
    public bool Equals(MeshHandle other) => Id == other.Id;
    public override bool Equals(object? obj) => obj is MeshHandle other && Equals(other);
    public override int GetHashCode() => Id.GetHashCode();
    public static bool operator ==(MeshHandle left, MeshHandle right) => left.Equals(right);
    public static bool operator !=(MeshHandle left, MeshHandle right) => !left.Equals(right);
    public static implicit operator ResourceHandle(MeshHandle value) => new(value.Id);
    public static explicit operator MeshHandle(ResourceHandle value) => new(value.Id);
}

[StructLayout(LayoutKind.Sequential)]
public readonly struct SkeletonHandle : IEquatable<SkeletonHandle>
{
    public static SkeletonHandle Invalid { get; } = new(ulong.MaxValue);
    public readonly ulong Id;
    public SkeletonHandle(ulong id) => Id = id;
    public bool IsValid => Id != ulong.MaxValue;
    public bool Equals(SkeletonHandle other) => Id == other.Id;
    public override bool Equals(object? obj) => obj is SkeletonHandle other && Equals(other);
    public override int GetHashCode() => Id.GetHashCode();
    public static bool operator ==(SkeletonHandle left, SkeletonHandle right) => left.Equals(right);
    public static bool operator !=(SkeletonHandle left, SkeletonHandle right) => !left.Equals(right);
    public static implicit operator ResourceHandle(SkeletonHandle value) => new(value.Id);
    public static explicit operator SkeletonHandle(ResourceHandle value) => new(value.Id);
}

[StructLayout(LayoutKind.Sequential)]
public readonly struct AnimationHandle : IEquatable<AnimationHandle>
{
    public static AnimationHandle Invalid { get; } = new(ulong.MaxValue);
    public readonly ulong Id;
    public AnimationHandle(ulong id) => Id = id;
    public bool IsValid => Id != ulong.MaxValue;
    public bool Equals(AnimationHandle other) => Id == other.Id;
    public override bool Equals(object? obj) => obj is AnimationHandle other && Equals(other);
    public override int GetHashCode() => Id.GetHashCode();
    public static bool operator ==(AnimationHandle left, AnimationHandle right) => left.Equals(right);
    public static bool operator !=(AnimationHandle left, AnimationHandle right) => !left.Equals(right);
    public static implicit operator ResourceHandle(AnimationHandle value) => new(value.Id);
    public static explicit operator AnimationHandle(ResourceHandle value) => new(value.Id);
}

[StructLayout(LayoutKind.Sequential)]
public readonly struct MaterialHandle : IEquatable<MaterialHandle>
{
    public static MaterialHandle Invalid { get; } = new(ulong.MaxValue);
    public readonly ulong Id;
    public MaterialHandle(ulong id) => Id = id;
    public bool IsValid => Id != ulong.MaxValue;
    public bool Equals(MaterialHandle other) => Id == other.Id;
    public override bool Equals(object? obj) => obj is MaterialHandle other && Equals(other);
    public override int GetHashCode() => Id.GetHashCode();
    public static bool operator ==(MaterialHandle left, MaterialHandle right) => left.Equals(right);
    public static bool operator !=(MaterialHandle left, MaterialHandle right) => !left.Equals(right);
    public static implicit operator ResourceHandle(MaterialHandle value) => new(value.Id);
    public static explicit operator MaterialHandle(ResourceHandle value) => new(value.Id);
}

[StructLayout(LayoutKind.Sequential)]
public readonly struct ShaderHandle : IEquatable<ShaderHandle>
{
    public static ShaderHandle Invalid { get; } = new(ulong.MaxValue);
    public readonly ulong Id;
    public ShaderHandle(ulong id) => Id = id;
    public bool IsValid => Id != ulong.MaxValue;
    public bool Equals(ShaderHandle other) => Id == other.Id;
    public override bool Equals(object? obj) => obj is ShaderHandle other && Equals(other);
    public override int GetHashCode() => Id.GetHashCode();
    public static bool operator ==(ShaderHandle left, ShaderHandle right) => left.Equals(right);
    public static bool operator !=(ShaderHandle left, ShaderHandle right) => !left.Equals(right);
    public static implicit operator ResourceHandle(ShaderHandle value) => new(value.Id);
    public static explicit operator ShaderHandle(ResourceHandle value) => new(value.Id);
}

[StructLayout(LayoutKind.Sequential)]
public readonly struct TextureHandle : IEquatable<TextureHandle>
{
    public static TextureHandle Invalid { get; } = new(ulong.MaxValue);
    public readonly ulong Id;
    public TextureHandle(ulong id) => Id = id;
    public bool IsValid => Id != ulong.MaxValue;
    public bool Equals(TextureHandle other) => Id == other.Id;
    public override bool Equals(object? obj) => obj is TextureHandle other && Equals(other);
    public override int GetHashCode() => Id.GetHashCode();
    public static bool operator ==(TextureHandle left, TextureHandle right) => left.Equals(right);
    public static bool operator !=(TextureHandle left, TextureHandle right) => !left.Equals(right);
    public static implicit operator ResourceHandle(TextureHandle value) => new(value.Id);
    public static explicit operator TextureHandle(ResourceHandle value) => new(value.Id);
}

[StructLayout(LayoutKind.Sequential)]
public readonly struct TextureSamplerHandle : IEquatable<TextureSamplerHandle>
{
    public static TextureSamplerHandle Invalid { get; } = new(ulong.MaxValue);
    public readonly ulong Id;
    public TextureSamplerHandle(ulong id) => Id = id;
    public bool IsValid => Id != ulong.MaxValue;
    public bool Equals(TextureSamplerHandle other) => Id == other.Id;
    public override bool Equals(object? obj) => obj is TextureSamplerHandle other && Equals(other);
    public override int GetHashCode() => Id.GetHashCode();
    public static bool operator ==(TextureSamplerHandle left, TextureSamplerHandle right) => left.Equals(right);
    public static bool operator !=(TextureSamplerHandle left, TextureSamplerHandle right) => !left.Equals(right);
    public static implicit operator ResourceHandle(TextureSamplerHandle value) => new(value.Id);
    public static explicit operator TextureSamplerHandle(ResourceHandle value) => new(value.Id);
}

[StructLayout(LayoutKind.Sequential)]
public readonly struct PhysicalMaterialHandle : IEquatable<PhysicalMaterialHandle>
{
    public static PhysicalMaterialHandle Invalid { get; } = new(ulong.MaxValue);
    public readonly ulong Id;
    public PhysicalMaterialHandle(ulong id) => Id = id;
    public bool IsValid => Id != ulong.MaxValue;
    public bool Equals(PhysicalMaterialHandle other) => Id == other.Id;
    public override bool Equals(object? obj) => obj is PhysicalMaterialHandle other && Equals(other);
    public override int GetHashCode() => Id.GetHashCode();
    public static bool operator ==(PhysicalMaterialHandle left, PhysicalMaterialHandle right) => left.Equals(right);
    public static bool operator !=(PhysicalMaterialHandle left, PhysicalMaterialHandle right) => !left.Equals(right);
    public static implicit operator ResourceHandle(PhysicalMaterialHandle value) => new(value.Id);
    public static explicit operator PhysicalMaterialHandle(ResourceHandle value) => new(value.Id);
}

[StructLayout(LayoutKind.Sequential)]
public readonly struct PrefabHandle : IEquatable<PrefabHandle>
{
    public static PrefabHandle Invalid { get; } = new(ulong.MaxValue);
    public readonly ulong Id;
    public PrefabHandle(ulong id) => Id = id;
    public bool IsValid => Id != ulong.MaxValue;
    public bool Equals(PrefabHandle other) => Id == other.Id;
    public override bool Equals(object? obj) => obj is PrefabHandle other && Equals(other);
    public override int GetHashCode() => Id.GetHashCode();
    public static bool operator ==(PrefabHandle left, PrefabHandle right) => left.Equals(right);
    public static bool operator !=(PrefabHandle left, PrefabHandle right) => !left.Equals(right);
    public static implicit operator ResourceHandle(PrefabHandle value) => new(value.Id);
    public static explicit operator PrefabHandle(ResourceHandle value) => new(value.Id);
}

[StructLayout(LayoutKind.Sequential)]
public readonly struct AnimationGraphHandle : IEquatable<AnimationGraphHandle>
{
    public static AnimationGraphHandle Invalid { get; } = new(ulong.MaxValue);
    public readonly ulong Id;
    public AnimationGraphHandle(ulong id) => Id = id;
    public bool IsValid => Id != ulong.MaxValue;
    public bool Equals(AnimationGraphHandle other) => Id == other.Id;
    public override bool Equals(object? obj) => obj is AnimationGraphHandle other && Equals(other);
    public override int GetHashCode() => Id.GetHashCode();
    public static bool operator ==(AnimationGraphHandle left, AnimationGraphHandle right) => left.Equals(right);
    public static bool operator !=(AnimationGraphHandle left, AnimationGraphHandle right) => !left.Equals(right);
    public static implicit operator ResourceHandle(AnimationGraphHandle value) => new(value.Id);
    public static explicit operator AnimationGraphHandle(ResourceHandle value) => new(value.Id);
}

[StructLayout(LayoutKind.Sequential)]
public readonly struct CubemapHandle : IEquatable<CubemapHandle>
{
    public static CubemapHandle Invalid { get; } = new(ulong.MaxValue);
    public readonly ulong Id;
    public CubemapHandle(ulong id) => Id = id;
    public bool IsValid => Id != ulong.MaxValue;
    public bool Equals(CubemapHandle other) => Id == other.Id;
    public override bool Equals(object? obj) => obj is CubemapHandle other && Equals(other);
    public override int GetHashCode() => Id.GetHashCode();
    public static bool operator ==(CubemapHandle left, CubemapHandle right) => left.Equals(right);
    public static bool operator !=(CubemapHandle left, CubemapHandle right) => !left.Equals(right);
    public static implicit operator ResourceHandle(CubemapHandle value) => new(value.Id);
    public static explicit operator CubemapHandle(ResourceHandle value) => new(value.Id);
}

[StructLayout(LayoutKind.Sequential)]
public readonly struct PhysicsCollisionMeshHandle : IEquatable<PhysicsCollisionMeshHandle>
{
    public static PhysicsCollisionMeshHandle Invalid { get; } = new(ulong.MaxValue);
    public readonly ulong Id;
    public PhysicsCollisionMeshHandle(ulong id) => Id = id;
    public bool IsValid => Id != ulong.MaxValue;
    public bool Equals(PhysicsCollisionMeshHandle other) => Id == other.Id;
    public override bool Equals(object? obj) => obj is PhysicsCollisionMeshHandle other && Equals(other);
    public override int GetHashCode() => Id.GetHashCode();
    public static bool operator ==(PhysicsCollisionMeshHandle left, PhysicsCollisionMeshHandle right) => left.Equals(right);
    public static bool operator !=(PhysicsCollisionMeshHandle left, PhysicsCollisionMeshHandle right) => !left.Equals(right);
    public static implicit operator ResourceHandle(PhysicsCollisionMeshHandle value) => new(value.Id);
    public static explicit operator PhysicsCollisionMeshHandle(ResourceHandle value) => new(value.Id);
}

[StructLayout(LayoutKind.Sequential)]
public readonly struct SpriteHandle : IEquatable<SpriteHandle>
{
    public static SpriteHandle Invalid { get; } = new(ulong.MaxValue);
    public readonly ulong Id;
    public SpriteHandle(ulong id) => Id = id;
    public bool IsValid => Id != ulong.MaxValue;
    public bool Equals(SpriteHandle other) => Id == other.Id;
    public override bool Equals(object? obj) => obj is SpriteHandle other && Equals(other);
    public override int GetHashCode() => Id.GetHashCode();
    public static bool operator ==(SpriteHandle left, SpriteHandle right) => left.Equals(right);
    public static bool operator !=(SpriteHandle left, SpriteHandle right) => !left.Equals(right);
    public static implicit operator ResourceHandle(SpriteHandle value) => new(value.Id);
    public static explicit operator SpriteHandle(ResourceHandle value) => new(value.Id);
}

[StructLayout(LayoutKind.Sequential)]
public readonly struct CurveHandle : IEquatable<CurveHandle>
{
    public static CurveHandle Invalid { get; } = new(ulong.MaxValue);
    public readonly ulong Id;
    public CurveHandle(ulong id) => Id = id;
    public bool IsValid => Id != ulong.MaxValue;
    public bool Equals(CurveHandle other) => Id == other.Id;
    public override bool Equals(object? obj) => obj is CurveHandle other && Equals(other);
    public override int GetHashCode() => Id.GetHashCode();
    public static bool operator ==(CurveHandle left, CurveHandle right) => left.Equals(right);
    public static bool operator !=(CurveHandle left, CurveHandle right) => !left.Equals(right);
    public static implicit operator ResourceHandle(CurveHandle value) => new(value.Id);
    public static explicit operator CurveHandle(ResourceHandle value) => new(value.Id);
}
