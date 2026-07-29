using System.Collections.Generic;
using SFG;

public sealed class ApiTestChecks
{
    private enum State
    {
        Pending,
        Passed,
        Failed,
    }

    private sealed class Entry
    {
        public required string Name;
        public string Detail = "waiting";
        public State State;
    }

    private readonly List<Entry> _entries = new();
    private readonly Dictionary<string, Entry> _entriesByName = new();

    public int PassedCount { get; private set; }
    public int FailedCount { get; private set; }
    public int PendingCount => _entries.Count - PassedCount - FailedCount;

    public void Define(string name, string detail = "waiting")
    {
        if (_entriesByName.ContainsKey(name))
        {
            return;
        }

        Entry entry = new()
        {
            Name = name,
            Detail = detail,
        };

        _entries.Add(entry);
        _entriesByName.Add(name, entry);
    }

    public void Pass(string name, string detail = "ok")
    {
        Set(name, State.Passed, detail);
    }

    public void Fail(string name, string detail)
    {
        Set(name, State.Failed, detail);
    }

    public void Draw(World world, string controls)
    {
        return;
        world.DebugDrawText2D(
            new Vector2(18.0f, 18.0f),
            $"SFG API TEST  PASS {PassedCount}  FAIL {FailedCount}  WAIT {PendingCount}",
            FailedCount == 0 ? Color.Cyan : Color.Red,
            17.0f);

        float y = 46.0f;

        foreach (Entry entry in _entries)
        {
            Color color = entry.State switch
            {
                State.Passed => Color.Green,
                State.Failed => Color.Red,
                _ => Color.Yellow,
            };
            string marker = entry.State switch
            {
                State.Passed => "PASS",
                State.Failed => "FAIL",
                _ => "WAIT",
            };

            world.DebugDrawText2D(
                new Vector2(18.0f, y),
                $"{marker,-4}  {entry.Name}: {entry.Detail}",
                color,
                13.0f);
            y += 17.0f;
        }

        world.DebugDrawText2D(new Vector2(18.0f, y + 8.0f), controls, Color.White, 13.0f);
    }

    private void Set(string name, State state, string detail)
    {
        if (!_entriesByName.TryGetValue(name, out Entry? entry))
        {
            Define(name);
            entry = _entriesByName[name];
        }

        if (entry.State == state && entry.Detail == detail)
        {
            return;
        }

        if (entry.State == State.Passed)
        {
            PassedCount--;
        }
        else if (entry.State == State.Failed)
        {
            FailedCount--;
        }

        entry.State = state;
        entry.Detail = detail;

        if (state == State.Passed)
        {
            PassedCount++;
        }
        else
        {
            FailedCount++;
        }
    }
}
