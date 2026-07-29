using System.Runtime.InteropServices;
using SFG;

[Component(6740644036173207099UL)]
[StructLayout(LayoutKind.Sequential)]
public struct ApiTestConfig
{
    public EntityGuid CanvasEntity;
    public PrefabHandle CratePrefab;
    public MaterialHandle TestMaterial;
    public TextureHandle TextureA;
    public TextureHandle TextureB;
    public TextureSamplerHandle SamplerA;
    public TextureSamplerHandle SamplerB;
    public float MoveSpeed;
    public float JumpSpeed;
    public float CastRadius;
    public float CrateImpulse;
}

[Component]
[StructLayout(LayoutKind.Sequential)]
public struct ApiTestProbe
{
    public Vector3 Position;
    public EntityGuid Owner;
    public MaterialHandle Material;
    public float Value;
    public int Sequence;
}
