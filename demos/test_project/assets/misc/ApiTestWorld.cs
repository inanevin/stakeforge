using SFG;
using SFG.Components;

public sealed class ApiTestWorld : WorldScript
{
    private const ulong PlayerTag = 1UL << 0;
    private const ulong TestTriggerTag = 1UL << 1;
    private const ulong CastTargetTag = 1UL << 2;
    private const ulong RuntimeTestTag = 1UL << 8;

    private const ushort KeySpace = 0x20;
    private const ushort KeyDelete = 0x2E;
    private const ushort KeyA = 0x41;
    private const ushort KeyD = 0x44;
    private const ushort KeyE = 0x45;
    private const ushort KeyH = 0x48;
    private const ushort KeyK = 0x4B;
    private const ushort KeyM = 0x4D;
    private const ushort KeyR = 0x52;
    private const ushort KeyS = 0x53;
    private const ushort KeyT = 0x54;
    private const ushort KeyW = 0x57;

    private static readonly ulong SpeedParameter = Hash.StringId("speed");
    private static readonly ulong GroundedParameter = Hash.StringId("grounded");
    private static readonly ulong RoughnessParameter = Hash.StringId("roughness_multiplier");
    private static readonly ulong AlbedoTexture = Hash.StringId("albedo");
    private static readonly ulong AlbedoSampler = Hash.StringId("albedo");

    private readonly ApiTestChecks _checks = new();
    private readonly Entity[] _entityBuffer = new Entity[32];
    private readonly PhysicsHit[] _hitBuffer = new PhysicsHit[32];
    private readonly PhysicsLinecast[] _batchLines = new PhysicsLinecast[3];
    private readonly PhysicsHit[] _batchHits = new PhysicsHit[3];

    private ApiTestConfig _config;
    private Entity _configEntity = Entity.Invalid;
    private Entity _player = Entity.Invalid;
    private Entity _animationEntity = Entity.Invalid;
    private Entity _triggerZone = Entity.Invalid;
    private Entity _materialTarget = Entity.Invalid;
    private Entity _castTarget = Entity.Invalid;
    private Entity _helperEntity = Entity.Invalid;
    private Entity _spawnedCrate = Entity.Invalid;
    private Entity _canvasEntity = Entity.Invalid;
    private CanvasWidget _canvasStatus = CanvasWidget.Invalid;
    private CanvasWidget _spawnButton = CanvasWidget.Invalid;
    private CanvasWidget _timeButton = CanvasWidget.Invalid;
    private CanvasWidget _materialButton = CanvasWidget.Invalid;
    private CanvasWidget _debugButton = CanvasWidget.Invalid;
    private PhysicsHit _lastCastHit;
    private Vector3 _lastCastOrigin = Vector3.Zero;
    private Vector3 _lastCastEnd = Vector3.Zero;
    private float _animationSpeed;
    private float _animationSpeedVelocity;
    private bool _moveForward;
    private bool _moveBackward;
    private bool _moveLeft;
    private bool _moveRight;
    private bool _jumpQueued;
    private bool _materialAlternate;
    private bool _materialHidden;
    private bool _castsExecuted;
    private bool _spawnImpulsePending;
    private bool _hasLastCastHit;
    private bool _debugDrawingEnabled = true;

    public override void BeginPlay(World world)
    {
        DefineChecks();

        FindAuthoredEntities(world);
        CreateCanvasWidgets(world);
        RunWorldChecks(world);
        RunAnimationCheck(world);
        RunMaterialChecks(false);
        SpawnCrate(world);
    }

    public override void Tick(World world, float deltaTime)
    {
        UpdateCharacter(world, deltaTime);
        UpdateSpawnedCrate(world);
    }

    public override void PostTick(World world, float deltaTime)
    {
    }

    public override void DrawDebug(World world)
    {
        if (!_debugDrawingEnabled)
        {
            return;
        }

        DrawWorldDebug(world);

        if (_helperEntity.IsValid && world.IsAlive(_helperEntity))
        {
           world.DebugDrawText3D(new Vector3(0.0f, 1.0f, 0.0f), "runtime ECS entity", Color.Cyan, 13.0f);
        }

      _checks.Pass("world/time", "scaled and real clocks available");
       _checks.Draw(
           world,
           $"WASD move | Space jump | E spawn crate | LMB ray | MMB line | RMB sphere | M material | H hide/show | T timescale | R reload | K load new_w | Delete crate | time {world.ElapsedTime:0.00}/{world.RealElapsedTime:0.00}");
    }

    public override void PostPhysicsTick(World world, float deltaTime)
    {
        if (!_castsExecuted)
        {
            RunAllCasts(world);
            _castsExecuted = true;
        }
    }

    public override void PostAnimationTick(World world, float deltaTime)
    {
        if (!_animationEntity.IsValid)
        {
            return;
        }

        bool speedRead = Animation.TryGetGraphParameter(world, _animationEntity, SpeedParameter, out float speed);
        bool groundedRead = Animation.TryGetGraphParameter(world, _animationEntity, GroundedParameter, out bool grounded);

        if (speedRead && groundedRead)
        {
            _checks.Pass("animation/parameters", $"speed {speed:0.00}, grounded {grounded}");
        }
        else
        {
            _checks.Fail("animation/parameters", "could not read speed or grounded");
        }
    }

    public override void OnKeyEvent(World world, KeyEvent inputEvent)
    {
        _checks.Pass("input/key", $"key 0x{inputEvent.Key:X}, {inputEvent.Action}");

        bool down = inputEvent.Action != InputAction.Release;

        switch (inputEvent.Key)
        {
            case KeyW:
                _moveForward = down;
                break;
            case KeyS:
                _moveBackward = down;
                break;
            case KeyA:
                _moveLeft = down;
                break;
            case KeyD:
                _moveRight = down;
                break;
        }

        if (inputEvent.Action != InputAction.Press)
        {
            return;
        }

        switch (inputEvent.Key)
        {
            case KeySpace:
                _jumpQueued = true;
                break;
            case KeyE:
                SpawnCrate(world);
                break;
            case KeyH:
                ToggleMaterialTargetVisibility(world);
                break;
            case KeyM:
                _materialAlternate = !_materialAlternate;
                RunMaterialChecks(_materialAlternate);
                break;
            case KeyR:
                Game.RestartWorld();
                break;
            case KeyK:
                Game.LoadWorld(Hash.StringId("new_w"));
                break;
            case KeyT:
                world.TimeScale = world.TimeScale < 0.9f ? 1.0f : 0.25f;
                _checks.Pass("world/timescale", $"set to {world.TimeScale:0.00}");
                break;
            case KeyDelete:
                DestroySpawnedCrate(world);
                break;
        }
    }

    public override void OnMouseButtonEvent(World world, MouseButtonEvent inputEvent)
    {
        _checks.Pass("input/mouse", $"{inputEvent.Button}, {inputEvent.Action}");

        if (inputEvent.Action != InputAction.Press)
        {
            return;
        }

        switch (inputEvent.Button)
        {
            case MouseButton.Left:
                RunRaycasts(world);
                break;
            case MouseButton.Middle:
                RunLinecasts(world);
                break;
            case MouseButton.Right:
                RunSpherecasts(world);
                break;
        }
    }

    public override void OnMouseMoveEvent(World world, MouseMoveEvent inputEvent)
    {
        if (!inputEvent.Delta.IsZero())
        {
            _checks.Pass("input/mouse", $"move {inputEvent.Delta}");
        }
    }

    public override void OnMouseWheelEvent(World world, MouseWheelEvent inputEvent)
    {
        _checks.Pass("input/mouse", $"wheel {inputEvent.Delta:0.00}");
    }

    public override void OnCanvasEvent(World world, CanvasEvent canvasEvent)
    {
        _checks.Pass("input/canvas", $"{canvasEvent.Type} widget {canvasEvent.WidgetHandle}");

        if (canvasEvent.Type != CanvasEventType.Click)
        {
            return;
        }

        if (canvasEvent.Widget == _spawnButton)
        {
            SpawnCrate(world);
            Canvas.SetText(world, _canvasStatus, "spawned a physics crate");
        }
        else if (canvasEvent.Widget == _timeButton)
        {
            world.TimeScale = world.TimeScale < 0.9f ? 1.0f : 0.25f;
            Canvas.SetText(world, _canvasStatus, $"time scale {world.TimeScale:0.00}");
        }
        else if (canvasEvent.Widget == _materialButton)
        {
            _materialAlternate = !_materialAlternate;
            RunMaterialChecks(_materialAlternate);
            Canvas.SetText(world, _canvasStatus, _materialAlternate ? "alternate material values" : "default material values");
        }
        else if (canvasEvent.Widget == _debugButton)
        {
            _debugDrawingEnabled = !_debugDrawingEnabled;
            Canvas.SetText(world, _debugButton, _debugDrawingEnabled ? "HIDE DEBUG DRAW" : "SHOW DEBUG DRAW");
            Canvas.SetText(world, _canvasStatus, _debugDrawingEnabled ? "debug drawing enabled" : "debug drawing hidden");
        }
    }

    public override void OnCollisionEnter(World world, PhysicsContactEvent contact)
    {
        _checks.Pass("physics/collision enter", ContactDetail(contact));
    }

    public override void OnCollisionStay(World world, PhysicsContactEvent contact)
    {
        _checks.Pass("physics/collision stay", ContactDetail(contact));
    }

    public override void OnCollisionExit(World world, PhysicsContactEvent contact)
    {
        _checks.Pass("physics/collision exit", ContactDetail(contact));
    }

    public override void OnTriggerEnter(World world, PhysicsContactEvent contact)
    {
        _checks.Pass("physics/trigger enter", ContactDetail(contact));
    }

    public override void OnTriggerStay(World world, PhysicsContactEvent contact)
    {
        _checks.Pass("physics/trigger stay", ContactDetail(contact));
    }

    public override void OnTriggerExit(World world, PhysicsContactEvent contact)
    {
        _checks.Pass("physics/trigger exit", ContactDetail(contact));
    }

    public override void EndPlay(World world)
    {
    }

    private void DefineChecks()
    {
        _checks.Define("setup/config");
        _checks.Define("setup/player");
        _checks.Define("setup/animation");
        _checks.Define("setup/trigger");
        _checks.Define("setup/cast targets");
        _checks.Define("setup/material target");
        _checks.Define("world/entity lifecycle");
        _checks.Define("world/transforms");
        _checks.Define("world/component CRUD");
        _checks.Define("world/tags");
        _checks.Define("world/guid lookup");
        _checks.Define("world/query");
        _checks.Define("world/prefab");
        _checks.Define("world/visibility");
        _checks.Define("world/time");
        _checks.Define("world/timescale");
        _checks.Define("physics/character");
        _checks.Define("physics/body");
        _checks.Define("physics/raycast");
        _checks.Define("physics/linecast");
        _checks.Define("physics/spherecast");
        _checks.Define("physics/collision enter");
        _checks.Define("physics/collision stay");
        _checks.Define("physics/collision exit");
        _checks.Define("physics/trigger enter");
        _checks.Define("physics/trigger stay");
        _checks.Define("physics/trigger exit");
        _checks.Define("animation/parameters");
        _checks.Define("resource/material float");
        _checks.Define("resource/material texture");
        _checks.Define("resource/material sampler");
        _checks.Define("input/key");
        _checks.Define("input/mouse");
        _checks.Define("input/canvas");
    }

    private void FindAuthoredEntities(World world)
    {
        _configEntity = world.GetEntityWithComponent(ComponentType<ApiTestConfig>.Value);

        if (_configEntity.IsValid &&
            world.TryGetComponent(_configEntity, ComponentType<ApiTestConfig>.Value, out _config))
        {
            _checks.Pass("setup/config", $"entity {_configEntity.Id}");
        }
        else
        {
            _checks.Fail("setup/config", "ApiTestConfig was not found");
        }

        _player = world.GetEntityWithTag(PlayerTag);

        if (_player.IsValid &&
            world.HasComponent(_player, ComponentType<CharacterMoverComponent>.Value))
        {
            _checks.Pass("setup/player", $"entity {_player.Id}");
        }
        else
        {
            _checks.Fail("setup/player", "player tag or CharacterMover is missing");
        }

        _animationEntity = world.GetEntityWithComponent(ComponentType<AnimationGraphComponent>.Value);

        if (_animationEntity.IsValid)
        {
            _checks.Pass("setup/animation", $"entity {_animationEntity.Id}");
        }
        else
        {
            _checks.Fail("setup/animation", "AnimationGraphComponent was not found");
        }

        _triggerZone = world.GetEntityWithTag(TestTriggerTag);

        if (_triggerZone.IsValid)
        {
            _checks.Pass("setup/trigger", $"entity {_triggerZone.Id}");
        }
        else
        {
            _checks.Fail("setup/trigger", "test_trigger tag was not found");
        }

        int castTargetCount = world.GetEntitiesWithTag(CastTargetTag, _entityBuffer);

        if (castTargetCount > 0)
        {
            _castTarget = _entityBuffer[0];
            _checks.Pass("setup/cast targets", $"{castTargetCount} found");
        }
        else
        {
            _checks.Fail("setup/cast targets", "cast_target tag was not found");
        }

        _materialTarget = world.GetEntityWithName("MaterialTarget");

        if (_materialTarget.IsValid)
        {
            _checks.Pass("setup/material target", $"entity {_materialTarget.Id}");
        }
        else
        {
            _checks.Fail("setup/material target", "MaterialTarget was not found");
        }
    }

    private void CreateCanvasWidgets(World world)
    {
        _canvasEntity = world.FindEntityByGuid(_config.CanvasEntity);

        if (!_canvasEntity.IsValid || !world.IsAlive(_canvasEntity))
        {
            _checks.Fail("input/canvas", "assign a Canvas entity to ApiTestConfig");
            return;
        }

        CanvasFrameStyle panelStyle = new(new Color(0.025f, 0.035f, 0.055f, 0.94f))
        {
            OutlineColor = new Color(0.15f, 0.7f, 0.95f, 1.0f),
            OutlineThickness = 1.0f,
            Rounding = 8.0f,
            RoundingSegments = 8,
            AntiAliasThickness = 1.0f,
        };
        CanvasLayout panelLayout = CanvasLayout.Fixed(320.0f, 292.0f);
        panelLayout.PositionModeX = CanvasPositionMode.AbsoluteScreen;
        panelLayout.PositionModeY = CanvasPositionMode.AbsoluteScreen;
        panelLayout.PositionValue = new Vector2(18.0f, 18.0f);
        panelLayout.ChildMargins = new Vector4(14.0f, 14.0f, 14.0f, 14.0f);
        panelLayout.ChildSpacing = 8.0f;
        panelLayout.Flow = CanvasFlow.Column;
        panelLayout.ChildClipMode = CanvasClipMode.ScissorRect;

        CanvasWidget panel = Canvas.CreateFrame(world, _canvasEntity, panelLayout, panelStyle);

        if (!panel.IsValid)
        {
            _checks.Fail("input/canvas", "Canvas component runtime is unavailable");
            return;
        }

        CanvasTextStyle titleStyle = new(20.0f, Color.White);
        CanvasTextStyle statusStyle = new(14.0f, Color.Cyan);
        CanvasLayout titleLayout = CanvasLayout.Fixed(292.0f, 28.0f);
        CanvasLayout statusLayout = CanvasLayout.Fixed(292.0f, 24.0f);

        Canvas.CreateText(world, _canvasEntity, titleLayout, "STAKEFORGE CANVAS", titleStyle, panel);
        _canvasStatus = Canvas.CreateText(world, _canvasEntity, statusLayout, "click a test action", statusStyle, panel);

        CanvasFrameStyle buttonStyle = new(new Color(0.08f, 0.12f, 0.18f, 1.0f))
        {
            OutlineColor = new Color(0.18f, 0.3f, 0.42f, 1.0f),
            OutlineThickness = 1.0f,
            Rounding = 5.0f,
            RoundingSegments = 6,
            AntiAliasThickness = 1.0f,
        };
        CanvasTextStyle buttonTextStyle = new(15.0f, Color.White);
        CanvasLayout buttonLayout = CanvasLayout.Fixed(292.0f, 38.0f);
        Color hoverColor = new(0.12f, 0.42f, 0.62f, 1.0f);
        Color pressColor = new(0.06f, 0.24f, 0.38f, 1.0f);

        _spawnButton = Canvas.CreateButton(world, _canvasEntity, buttonLayout, "SPAWN CRATE", buttonStyle, buttonTextStyle, hoverColor, pressColor, panel);
        _timeButton = Canvas.CreateButton(world, _canvasEntity, buttonLayout, "TOGGLE TIME SCALE", buttonStyle, buttonTextStyle, hoverColor, pressColor, panel);
        _materialButton = Canvas.CreateButton(world, _canvasEntity, buttonLayout, "TOGGLE MATERIAL", buttonStyle, buttonTextStyle, hoverColor, pressColor, panel);
        _debugButton = Canvas.CreateButton(world, _canvasEntity, buttonLayout, "HIDE DEBUG DRAW", buttonStyle, buttonTextStyle, hoverColor, pressColor, panel);

        if (_spawnButton.IsValid && _timeButton.IsValid && _materialButton.IsValid && _debugButton.IsValid)
        {
            _checks.Pass("input/canvas", "panel and buttons created");
        }
        else
        {
            _checks.Fail("input/canvas", "one or more widgets could not be created");
        }
    }

    private void RunWorldChecks(World world)
    {
        _helperEntity = world.CreateEntity("ApiRuntimeHelper");
        Entity duplicate = world.DuplicateEntity(_helperEntity);
        bool attached = _player.IsValid && world.AttachEntity(_helperEntity, _player);
        bool detached = attached && world.DetachEntity(_helperEntity);
        bool duplicateDestroyed = duplicate.IsValid && world.DestroyEntity(duplicate);

        if (_helperEntity.IsValid && duplicate.IsValid && detached && duplicateDestroyed)
        {
            _checks.Pass("world/entity lifecycle", "create, duplicate, attach, detach, destroy");
        }
        else
        {
            _checks.Fail("world/entity lifecycle", "one or more operations failed");
        }

        Vector3 testPosition = new(2.0f, 1.0f, -2.0f);
        Quaternion testRotation = Quaternion.FromEuler(0.0f, 35.0f, 0.0f);
        Vector3 testScale = new(0.75f, 0.75f, 0.75f);
        bool positionSet = world.SetEntityPositionLocal(_helperEntity, testPosition);
        bool rotationSet = world.SetEntityRotationLocal(_helperEntity, testRotation);
        bool scaleSet = world.SetEntityScaleLocal(_helperEntity, testScale);
        bool positionRead = world.TryGetEntityPositionLocal(_helperEntity, out Vector3 readPosition);
        bool rotationRead = world.TryGetEntityRotationLocal(_helperEntity, out Quaternion readRotation);
        bool scaleRead = world.TryGetEntityScaleLocal(_helperEntity, out Vector3 readScale);
        bool converted = world.TryConvertAbsolutePositionToLocal(_helperEntity, readPosition, out Vector3 localPosition);

        if (positionSet && rotationSet && scaleSet && positionRead && rotationRead && scaleRead && converted &&
            readPosition.ApproximatelyEquals(testPosition) && readScale.ApproximatelyEquals(testScale))
        {
            _checks.Pass("world/transforms", $"position {readPosition}, local conversion {localPosition}");
        }
        else
        {
            _checks.Fail("world/transforms", "set, get, or absolute conversion failed");
        }

        EntityGuid owner = EntityGuid.Invalid;

        if (_player.IsValid &&
            world.TryGetComponent(_player, ComponentType<GuidComponent>.Value, out GuidComponent playerGuid))
        {
            owner = playerGuid.Guid;
        }

        ApiTestProbe probe = new()
        {
            Position = testPosition,
            Owner = owner,
            Material = _config.TestMaterial,
            Value = 10.0f,
            Sequence = 1,
        };
        bool componentAdded = world.AddComponent(_helperEntity, ComponentType<ApiTestProbe>.Value, probe);
        bool componentRead = world.TryGetComponent(_helperEntity, ComponentType<ApiTestProbe>.Value, out ApiTestProbe readProbe);
        readProbe.Value = 20.0f;
        readProbe.Sequence = 2;
        bool componentSet = componentRead &&
                            world.SetComponent(_helperEntity, ComponentType<ApiTestProbe>.Value, readProbe);
        bool componentReadAgain = world.TryGetComponent(
            _helperEntity,
            ComponentType<ApiTestProbe>.Value,
            out ApiTestProbe updatedProbe);
        bool componentRemoved = world.RemoveComponent(_helperEntity, ComponentType<ApiTestProbe>.Value);

        if (componentAdded && componentSet && componentReadAgain &&
            updatedProbe.Value == 20.0f && updatedProbe.Sequence == 2 && componentRemoved)
        {
            _checks.Pass("world/component CRUD", "add, get, set, remove");
        }
        else
        {
            _checks.Fail("world/component CRUD", "script component operation failed");
        }

        bool tagSet = world.SetEntityTag(_helperEntity, RuntimeTestTag, true);
        Entity taggedEntity = world.GetEntityWithTag(RuntimeTestTag);
        bool tagRemoved = world.SetEntityTag(_helperEntity, RuntimeTestTag, false);

        if (tagSet && taggedEntity == _helperEntity && tagRemoved)
        {
            _checks.Pass("world/tags", "set, find, remove");
        }
        else
        {
            _checks.Fail("world/tags", "runtime tag operation failed");
        }

        if (owner.IsValid && world.FindEntityByGuid(owner) == _player)
        {
            _checks.Pass("world/guid lookup", $"player guid {owner.Id}");
        }
        else
        {
            _checks.Fail("world/guid lookup", "player GuidComponent lookup failed");
        }

        bool queryFoundPlayer = false;
        int queryCount = 0;

        foreach (WorldQueryRow row in world
                     .Query<TransformComponent>()
                     .Optional<CharacterMoverComponent>()
                     .Exclude<DisabledComponent>())
        {
            queryCount++;

            if (row.Entity == _player && row.GetOptional<CharacterMoverComponent>().HasValue)
            {
                queryFoundPlayer = true;
            }
        }

        if (queryFoundPlayer)
        {
            _checks.Pass("world/query", $"{queryCount} transforms, optional CharacterMover found");
        }
        else
        {
            _checks.Fail("world/query", "required/optional/excluded query missed player");
        }
    }

    private void RunAnimationCheck(World world)
    {
        if (!_animationEntity.IsValid)
        {
            return;
        }

        bool speedSet = Animation.SetGraphParameter(world, _animationEntity, SpeedParameter, 0.0f);
        bool groundedSet = Animation.SetGraphParameter(world, _animationEntity, GroundedParameter, true);

        if (!speedSet || !groundedSet)
        {
            _checks.Fail("animation/parameters", "speed or grounded parameter was not found");
        }
    }

    private void UpdateCharacter(World world, float deltaTime)
    {
        if (!_player.IsValid ||
            !Physics.TryGetCharacterState(world, _player, out CharacterMoverState state))
        {
            _checks.Fail("physics/character", "CharacterMover state is unavailable");
            _jumpQueued = false;
            return;
        }

        Vector3 input = Vector3.Zero;

        if (_moveForward)
        {
            input += Vector3.Forward;
        }

        if (_moveBackward)
        {
            input += Vector3.Back;
        }

        if (_moveLeft)
        {
            input += Vector3.Left;
        }

        if (_moveRight)
        {
            input += Vector3.Right;
        }

        float moveSpeed = _config.MoveSpeed > 0.0f ? _config.MoveSpeed : 5.0f;
        Vector3 horizontalVelocity = input.Normalized * moveSpeed;
        Vector3 velocity = new(horizontalVelocity.X, state.Velocity.Y, horizontalVelocity.Z);
        bool velocitySet = Physics.SetCharacterVelocity(world, _player, velocity);

        if (_jumpQueued && state.IsGrounded)
        {
            float jumpSpeed = _config.JumpSpeed > 0.0f ? _config.JumpSpeed : 6.0f;

            if (Physics.JumpCharacter(world, _player, jumpSpeed))
            {
                _checks.Pass("physics/character", $"move and jump, grounded {state.IsGrounded}");
            }

            _jumpQueued = false;
        }
        else if (velocitySet)
        {
            _checks.Pass("physics/character", $"velocity {velocity}, grounded {state.IsGrounded}");
        }

        if (_animationEntity.IsValid)
        {
            float targetAnimationSpeed = input.IsZero() ? 0.0f : 1.0f;
            float smoothTime = targetAnimationSpeed > _animationSpeed ? 0.12f : 0.2f;
            _animationSpeed = Ease.SmoothDamp(
                _animationSpeed,
                targetAnimationSpeed,
                ref _animationSpeedVelocity,
                smoothTime,
                10.0f,
                deltaTime);
            _animationSpeed = SFG.Math.Clamp(_animationSpeed, 0.0f, 1.0f);

            Animation.SetGraphParameter(world, _animationEntity, SpeedParameter, _animationSpeed);
            Animation.SetGraphParameter(world, _animationEntity, GroundedParameter, state.IsGrounded);
        }
    }

    private void SpawnCrate(World world)
    {
        if (_config.CratePrefab.Id == 0 || !_config.CratePrefab.IsValid)
        {
            
            _checks.Fail("world/prefab", "assign TestCrate to CratePrefab");
            return;
        }

        if (_spawnedCrate.IsValid && world.IsAlive(_spawnedCrate))
        {
            world.DestroyEntity(_spawnedCrate);
        }

        Vector3 spawnPosition = new(0.0f, 3.0f, 0.0f);

        if (_player.IsValid &&
            world.TryGetEntityPositionLastAbsolute(_player, out Vector3 playerPosition))
        {
            spawnPosition = playerPosition + Vector3.Up * 2.5f + Vector3.Right * 1.5f;
        }

        _spawnedCrate = world.SpawnPrefab(
            _config.CratePrefab,
            Entity.Invalid,
            spawnPosition,
            Quaternion.Identity,
            Vector3.One);
        _spawnImpulsePending = _spawnedCrate.IsValid;

        if (_spawnedCrate.IsValid)
        {
            _checks.Pass("world/prefab", $"spawned entity {_spawnedCrate.Id}");
        }
        else
        {
            _checks.Fail("world/prefab", "SpawnPrefab returned an invalid entity");
        }
    }

    private void UpdateSpawnedCrate(World world)
    {
        if (!_spawnImpulsePending || !_spawnedCrate.IsValid || !world.IsAlive(_spawnedCrate))
        {
            return;
        }

        float impulse = _config.CrateImpulse > 0.0f ? _config.CrateImpulse : 8.0f;
        bool woke = Physics.WakeBody(world, _spawnedCrate);
        bool linearSet = Physics.SetBodyLinearVelocity(world, _spawnedCrate, Vector3.Up);
        bool angularSet = Physics.SetBodyAngularVelocity(world, _spawnedCrate, new Vector3(0.0f, 2.0f, 1.0f));
        bool forceAdded = Physics.AddBodyForce(world, _spawnedCrate, Vector3.Right * 2.0f);
        bool impulseAdded = Physics.AddBodyImpulse(
            world,
            _spawnedCrate,
            (Vector3.Up + Vector3.Forward).Normalized * impulse);
        bool stateRead = Physics.TryGetBodyState(world, _spawnedCrate, out PhysicsBodyState state);

        if (woke && linearSet && angularSet && forceAdded && impulseAdded && stateRead)
        {
            _checks.Pass("physics/body", $"active {state.IsActive}, velocity {state.LinearVelocity}");
            _spawnImpulsePending = false;
        }
    }

    private void DestroySpawnedCrate(World world)
    {
        if (_spawnedCrate.IsValid && world.IsAlive(_spawnedCrate))
        {
            world.DestroyEntity(_spawnedCrate);
            _spawnedCrate = Entity.Invalid;
            _spawnImpulsePending = false;
        }
    }

    private void RunAllCasts(World world)
    {
        RunRaycasts(world);
        RunLinecasts(world);
        RunSpherecasts(world);
    }

    private void RunRaycasts(World world)
    {
        if (!TryBuildCast(world, out Vector3 origin, out Vector3 direction, out float distance))
        {
            _checks.Fail("physics/raycast", "player or cast target transform is unavailable");
            return;
        }

        PhysicsRaycast ray = new()
        {
            Origin = origin,
            Direction = direction,
            Distance = distance,
        };
        PhysicsQueryFilter filter = BuildCastFilter();
        bool any = Physics.RaycastAny(world, ray, filter);
        bool closest = Physics.RaycastClosest(world, ray, filter, out PhysicsHit closestHit);
        bool all = Physics.TryRaycastAll(world, ray, filter, _hitBuffer, out PhysicsQueryResult result);

        StoreCast(origin, origin + direction * distance, closest, closestHit);

        if (any && closest && all && result.HitCount > 0)
        {
            _checks.Pass("physics/raycast", $"{result.HitCount} hits, closest entity {closestHit.Entity.Id}");
        }
        else
        {
            _checks.Fail("physics/raycast", "no tagged CastTarget was hit");
        }
    }

    private void RunLinecasts(World world)
    {
        if (!TryBuildCast(world, out Vector3 origin, out Vector3 direction, out float distance))
        {
            _checks.Fail("physics/linecast", "player or cast target transform is unavailable");
            return;
        }

        Vector3 end = origin + direction * distance;
        PhysicsLinecast line = new()
        {
            Start = origin,
            End = end,
        };
        PhysicsQueryFilter filter = BuildCastFilter();
        bool closest = Physics.LinecastClosest(world, line, filter, out PhysicsHit closestHit);

        for (int lineIndex = 0; lineIndex < _batchLines.Length; lineIndex++)
        {
            float offset = lineIndex - 1.0f;
            _batchLines[lineIndex] = new PhysicsLinecast
            {
                Start = origin + Vector3.Up * offset * 0.15f,
                End = end + Vector3.Up * offset * 0.15f,
            };
        }

        Physics.LinecastClosestBatch(world, _batchLines, _batchHits, filter);

        int batchHitCount = 0;

        foreach (PhysicsHit hit in _batchHits)
        {
            if (hit.Entity.IsValid)
            {
                batchHitCount++;
            }
        }

        StoreCast(origin, end, closest, closestHit);

        if (closest && batchHitCount > 0)
        {
            _checks.Pass("physics/linecast", $"closest entity {closestHit.Entity.Id}, batch {batchHitCount}/3");
        }
        else
        {
            _checks.Fail("physics/linecast", "closest or batch cast missed");
        }
    }

    private void RunSpherecasts(World world)
    {
        if (!TryBuildCast(world, out Vector3 origin, out Vector3 direction, out float distance))
        {
            _checks.Fail("physics/spherecast", "player or cast target transform is unavailable");
            return;
        }

        float radius = _config.CastRadius > 0.0f ? _config.CastRadius : 0.5f;
        PhysicsSpherecast sphere = new()
        {
            Origin = origin,
            Direction = direction,
            Radius = radius,
            Distance = distance,
        };
        PhysicsQueryFilter filter = BuildCastFilter();
        bool any = Physics.SpherecastAny(world, sphere, filter);
        bool closest = Physics.SpherecastClosest(world, sphere, filter, out PhysicsHit closestHit);
        bool all = Physics.TrySpherecastAll(world, sphere, filter, _hitBuffer, out PhysicsQueryResult result);

        StoreCast(origin, origin + direction * distance, closest, closestHit);

        if (any && closest && all && result.HitCount > 0)
        {
            _checks.Pass("physics/spherecast", $"{result.HitCount} hits, radius {radius:0.00}");
        }
        else
        {
            _checks.Fail("physics/spherecast", "no tagged CastTarget was hit");
        }
    }

    private bool TryBuildCast(World world, out Vector3 origin, out Vector3 direction, out float distance)
    {
        origin = Vector3.Zero;
        direction = Vector3.Forward;
        distance = 0.0f;

        if (!_player.IsValid ||
            !_castTarget.IsValid ||
            !world.TryGetEntityPositionLastAbsolute(_player, out Vector3 playerPosition) ||
            !world.TryGetEntityPositionLastAbsolute(_castTarget, out Vector3 targetPosition))
        {
            return false;
        }

        origin = playerPosition + Vector3.Up * 0.75f;
        Vector3 castVector = targetPosition - origin;
        distance = castVector.Length + 1.0f;
        direction = castVector.Normalized;
        return distance > 1.0f;
    }

    private PhysicsQueryFilter BuildCastFilter()
    {
        PhysicsQueryFilter filter = PhysicsQueryFilter.Default;
        filter.RequiredAnyTags = CastTargetTag;
        filter.IgnoredEntity = _player;
        return filter;
    }

    private void StoreCast(Vector3 origin, Vector3 end, bool hit, PhysicsHit hitData)
    {
        _lastCastOrigin = origin;
        _lastCastEnd = end;
        _hasLastCastHit = hit;
        _lastCastHit = hitData;
    }

    private void RunMaterialChecks(bool alternate)
    {
        if (_config.TestMaterial.Id == 0 || !_config.TestMaterial.IsValid)
        {
            _checks.Fail("resource/material float", "assign target_mat to TestMaterial");
            _checks.Fail("resource/material texture", "assign TextureA and TextureB");
            _checks.Fail("resource/material sampler", "assign SamplerA and SamplerB");
            return;
        }

        float roughness = alternate ? 0.05f : 0.95f;

        if (Resource.UpdateMaterialParameter(_config.TestMaterial, RoughnessParameter, roughness))
        {
            _checks.Pass("resource/material float", $"roughness_multiplier {roughness:0.00}");
        }
        else
        {
            _checks.Fail("resource/material float", "roughness_multiplier update failed");
        }

        TextureHandle texture = alternate ? _config.TextureB : _config.TextureA;

        if (texture.Id != 0 && texture.IsValid &&
            Resource.UpdateMaterialTexture(_config.TestMaterial, AlbedoTexture, texture))
        {
            _checks.Pass("resource/material texture", $"albedo texture {texture.Id}");
        }
        else
        {
            _checks.Fail("resource/material texture", "assign TextureA and TextureB");
        }

        TextureSamplerHandle sampler = alternate ? _config.SamplerB : _config.SamplerA;

        if (sampler.Id != 0 && sampler.IsValid &&
            Resource.UpdateMaterialSampler(_config.TestMaterial, AlbedoSampler, sampler))
        {
            _checks.Pass("resource/material sampler", $"albedo sampler {sampler.Id}");
        }
        else
        {
            _checks.Fail("resource/material sampler", "assign SamplerA and SamplerB");
        }
    }

    private void ToggleMaterialTargetVisibility(World world)
    {
        if (!_materialTarget.IsValid)
        {
            return;
        }

        _materialHidden = !_materialHidden;
        bool changed = _materialHidden
            ? world.HideEntity(_materialTarget)
            : world.ShowEntity(_materialTarget);

        if (changed)
        {
            _checks.Pass("world/visibility", _materialHidden ? "hidden" : "shown");
        }
        else
        {
            _checks.Fail("world/visibility", "hide/show failed");
        }
    }

    private void DrawWorldDebug(World world)
    {
        if (_hasLastCastHit)
        {
            world.DebugDrawArrow(_lastCastOrigin, _lastCastHit.Position, Color.Green);
            world.DebugDrawSphere(_lastCastHit.Position, 0.16f, Color.Yellow);
            world.DebugDrawText3D(
                _lastCastHit.Position + Vector3.Up * 0.3f,
                $"hit {_lastCastHit.Entity.Id}",
                Color.Yellow,
                13.0f);
        }
        else if (!_lastCastOrigin.IsZero() || !_lastCastEnd.IsZero())
        {
            world.DebugDrawLine(_lastCastOrigin, _lastCastEnd, Color.Red);
        }

        if (_triggerZone.IsValid &&
            world.TryGetEntityPositionLastAbsolute(_triggerZone, out Vector3 triggerPosition))
        {
            world.DebugDrawAabb(
                triggerPosition - new Vector3(0.6f, 0.6f, 0.6f),
                triggerPosition + new Vector3(0.6f, 0.6f, 0.6f),
                Color.Magenta);
            world.DebugDrawText3D(triggerPosition + Vector3.Up, "trigger zone", Color.Magenta, 13.0f);
        }

        if (_castTarget.IsValid &&
            world.TryGetEntityPositionLastAbsolute(_castTarget, out Vector3 castTargetPosition))
        {
            world.DebugDrawCircle(castTargetPosition, 0.7f, Vector3.Up, Color.Cyan);
        }
    }

    private static string ContactDetail(PhysicsContactEvent contact)
    {
        return $"{contact.EntityA.Id} <-> {contact.EntityB.Id}";
    }
}
