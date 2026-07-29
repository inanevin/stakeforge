namespace SFG;

public static unsafe class Resource
{
    public static bool UpdateMaterialParameter(MaterialHandle material, ulong parameterHash, float value)
    {
        return ManagedRuntime.GetApi()->Resource->UpdateMaterialParameterF32(material.Id, parameterHash, value) != 0;
    }

    public static bool UpdateMaterialParameter(MaterialHandle material, ulong parameterHash, Vector2 value)
    {
        return ManagedRuntime.GetApi()->Resource->UpdateMaterialParameterVec2(material.Id, parameterHash, &value) != 0;
    }

    public static bool UpdateMaterialParameter(MaterialHandle material, ulong parameterHash, Vector4 value)
    {
        return ManagedRuntime.GetApi()->Resource->UpdateMaterialParameterVec4(material.Id, parameterHash, &value) != 0;
    }

    public static bool UpdateMaterialParameter(MaterialHandle material, ulong parameterHash, uint value)
    {
        return ManagedRuntime.GetApi()->Resource->UpdateMaterialParameterU32(material.Id, parameterHash, value) != 0;
    }

    public static bool UpdateMaterialTexture(MaterialHandle material, ulong textureHash, TextureHandle texture)
    {
        return ManagedRuntime.GetApi()->Resource->UpdateMaterialTexture(material.Id, textureHash, texture.Id) != 0;
    }

    public static bool UpdateMaterialSampler(MaterialHandle material, ulong samplerHash, TextureSamplerHandle sampler)
    {
        return ManagedRuntime.GetApi()->Resource->UpdateMaterialSampler(material.Id, samplerHash, sampler.Id) != 0;
    }
}
