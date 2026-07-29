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

    public virtual void OnKeyEvent(World world, KeyEvent inputEvent)
    {
    }

    public virtual void OnMouseButtonEvent(World world, MouseButtonEvent inputEvent)
    {
    }

    public virtual void OnMouseMoveEvent(World world, MouseMoveEvent inputEvent)
    {
    }

    public virtual void OnMouseWheelEvent(World world, MouseWheelEvent inputEvent)
    {
    }

    public virtual void OnCollisionEnter(World world, PhysicsContactEvent contact)
    {
    }

    public virtual void OnCollisionStay(World world, PhysicsContactEvent contact)
    {
    }

    public virtual void OnCollisionExit(World world, PhysicsContactEvent contact)
    {
    }

    public virtual void OnTriggerEnter(World world, PhysicsContactEvent contact)
    {
    }

    public virtual void OnTriggerStay(World world, PhysicsContactEvent contact)
    {
    }

    public virtual void OnTriggerExit(World world, PhysicsContactEvent contact)
    {
    }
}
