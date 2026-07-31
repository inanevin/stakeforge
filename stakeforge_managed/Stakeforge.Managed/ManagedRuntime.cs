using System;
using System.Runtime.InteropServices;

namespace SFG;

[StructLayout(LayoutKind.Sequential)]
internal unsafe struct NativeApi
{
    internal uint Size;
    internal uint Version;
    internal delegate* unmanaged[Cdecl]<byte*, void> LogInfo;
    internal delegate* unmanaged[Cdecl]<byte*, void> LogError;
    internal NativePlatformApi* Platform;
    internal NativeGameApi* Game;
    internal NativeStatsApi* Stats;
    internal NativeScreenApi* Screen;
    internal NativeResourceApi* Resource;
    internal NativeWorldApi* World;
    internal NativeAudioApi* Audio;
    internal NativePhysicsApi* Physics;
    internal NativeAnimationApi* Animation;
    internal NativeCanvasApi* Canvas;
    internal delegate* unmanaged[Cdecl]<byte*, void> GameLogInfo;
    internal delegate* unmanaged[Cdecl]<byte*, void> GameLogError;
    internal delegate* unmanaged[Cdecl]<byte*, void> GameLogWarn;
    internal delegate* unmanaged[Cdecl]<byte*, void> GameLogTrace;
}

internal static unsafe class ManagedRuntime
{
    private const uint ApiVersion = 8;
    private const uint CategoryApiVersion = 1;
    private const uint GameApiVersion = 4;
	private const uint WorldApiVersion = 8;

    private static NativeApi* _api;

    internal static int Initialize(void* apiAddress)
    {
        NativeApi* api = (NativeApi*)apiAddress;

        if (sizeof(Entity) != 4 ||
            sizeof(CollisionLayer) != 1 ||
            sizeof(CollisionLayerMask) != 8 ||
            sizeof(EntityGuid) != 8 ||
            sizeof(ResourceHandle) != 8 ||
            sizeof(AudioHandle) != 8 ||
            sizeof(FontHandle) != 8 ||
            sizeof(MeshHandle) != 8 ||
            sizeof(SkeletonHandle) != 8 ||
            sizeof(AnimationHandle) != 8 ||
            sizeof(MaterialHandle) != 8 ||
            sizeof(ShaderHandle) != 8 ||
            sizeof(TextureHandle) != 8 ||
            sizeof(TextureSamplerHandle) != 8 ||
            sizeof(PhysicalMaterialHandle) != 8 ||
            sizeof(PrefabHandle) != 8 ||
            sizeof(AnimationGraphHandle) != 8 ||
            sizeof(CubemapHandle) != 8 ||
            sizeof(PhysicsCollisionMeshHandle) != 8 ||
            sizeof(SpriteHandle) != 8 ||
            sizeof(CurveHandle) != 8 ||
            sizeof(RenderResolution) != 4 ||
            sizeof(Vector2) != 8 ||
            sizeof(Vector3) != 12 ||
            sizeof(Vector4) != 16 ||
            sizeof(Color) != 16 ||
            sizeof(Quaternion) != 16 ||
            sizeof(Matrix3x3) != 36 ||
            sizeof(Matrix4x3) != 48 ||
            sizeof(Matrix4x4) != 64 ||
            sizeof(PhysicsQueryFilter) != 40 ||
            sizeof(PhysicsRaycast) != 28 ||
            sizeof(PhysicsLinecast) != 24 ||
            sizeof(PhysicsSpherecast) != 32 ||
            sizeof(PhysicsHit) != 56 ||
            sizeof(PhysicsQueryResult) != 8 ||
            sizeof(PhysicsBodyState) != 56 ||
            sizeof(CharacterMoverState) != 48 ||
            sizeof(PhysicsContactEvent) != 56 ||
            sizeof(NativeWorldQueryComponent) != 16 ||
            sizeof(NativeWorldQuery) != 768 ||
            sizeof(NativeWorldQueryRow) != 272 ||
            sizeof(CanvasLayout) != 56 ||
            sizeof(CanvasFrameStyle) != 64 ||
            sizeof(CanvasTextStyle) != 32 ||
            sizeof(CanvasWidget) != 8 ||
            sizeof(CanvasEvent) != 36 ||
            Hash.StringId("move_speed") != 10935991027489123434UL ||
            Hash.StringId("\u00E9") != 11062259058118930795UL ||
            Hash.Fnv1A64("Stakeforge") != 3213806815071520380UL ||
            api == null ||
            api->Size < sizeof(NativeApi) ||
            api->Version != ApiVersion ||
            api->LogInfo == null ||
            api->LogError == null ||
            api->Platform == null ||
            api->Platform->Size < sizeof(NativePlatformApi) ||
            api->Platform->Version != CategoryApiVersion ||
            api->Game == null ||
            api->Game->Size < sizeof(NativeGameApi) ||
            api->Game->Version != GameApiVersion ||
            api->Stats == null ||
            api->Stats->Size < sizeof(NativeStatsApi) ||
            api->Stats->Version != CategoryApiVersion ||
            api->Screen == null ||
            api->Screen->Size < sizeof(NativeScreenApi) ||
            api->Screen->Version != CategoryApiVersion ||
            api->Resource == null ||
            api->Resource->Size < sizeof(NativeResourceApi) ||
            api->Resource->Version != CategoryApiVersion ||
            api->World == null ||
            api->World->Size < sizeof(NativeWorldApi) ||
            api->World->Version != WorldApiVersion ||
            api->Audio == null ||
            api->Audio->Size < sizeof(NativeAudioApi) ||
            api->Audio->Version != CategoryApiVersion ||
            api->Physics == null ||
            api->Physics->Size < sizeof(NativePhysicsApi) ||
            api->Physics->Version != CategoryApiVersion ||
            api->Animation == null ||
            api->Animation->Size < sizeof(NativeAnimationApi) ||
            api->Animation->Version != CategoryApiVersion ||
            api->Canvas == null ||
            api->Canvas->Size < sizeof(NativeCanvasApi) ||
            api->Canvas->Version != CategoryApiVersion ||
            api->GameLogInfo == null ||
            api->GameLogError == null ||
            api->GameLogWarn == null ||
            api->GameLogTrace == null)
        {
            return -1;
        }

        _api = api;
        LogInfo("managed scripting API initialized correctly.");
        return 0;
    }

    internal static void Shutdown()
    {
        _api = null;
    }

    internal static NativeApi* GetApi()
    {
        if (_api == null)
        {
            throw new InvalidOperationException("Stakeforge managed API is not initialized.");
        }

        return _api;
    }

    internal static void LogInfo(string message)
    {
        Log(GetApi()->LogInfo, message);
    }

    internal static void LogGameInfo(string message)
    {
        Log(GetApi()->GameLogInfo, message);
    }

    internal static void LogGameError(string message)
    {
        Log(GetApi()->GameLogError, message);
    }

    internal static void LogGameWarn(string message)
    {
        Log(GetApi()->GameLogWarn, message);
    }

    internal static void LogGameTrace(string message)
    {
        Log(GetApi()->GameLogTrace, message);
    }

    internal static void TryLogError(void* apiAddress, string message)
    {
        try
        {
            NativeApi* api = (NativeApi*)apiAddress;

            if (api != null && api->Size >= 24 && api->LogError != null)
            {
                Log(api->LogError, message);
            }
        }
        catch
        {
        }
    }

    internal static void TryLogError(string message)
    {
        TryLogError(_api, message);
    }

    internal static void TryLogError(void* apiAddress, Exception exception)
    {
        try
        {
            TryLogError(apiAddress, exception.ToString());
        }
        catch
        {
        }
    }

    internal static void TryLogError(Exception exception)
    {
        TryLogError(_api, exception);
    }

    private static void Log(delegate* unmanaged[Cdecl]<byte*, void> callback, string message)
    {
        nint utf8Message = Marshal.StringToCoTaskMemUTF8(message);

        try
        {
            callback((byte*)utf8Message);
        }
        finally
        {
            Marshal.FreeCoTaskMem(utf8Message);
        }
    }
}
