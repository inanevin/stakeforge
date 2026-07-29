namespace SFG;

public abstract class WorldScript
{
    public virtual void BeginPlay(World world)
    {
    }

    public virtual void Tick(World world, float deltaTime)
    {
    }

    public virtual void PostTick(World world, float deltaTime)
    {
    }

    public virtual void PostPhysicsTick(World world, float deltaTime)
    {
    }

    public virtual void PostAnimationTick(World world, float deltaTime)
    {
    }

    public virtual void EndPlay(World world)
    {
    }
}
