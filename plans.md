# VorpalEngine — Future Plans

## Scene Config Files

Scenes as data-driven config (Lua table) that lists metadata, entry script, spawn location, and assets to preload.

```lua
-- example: assets/scenes/hunted.lua
return {
    name    = "HUNTED",
    script  = "assets/scripts/game.lua",
    spawn   = { x=0, y=2, z=0 },
    preload = {
        "assets/models/zombie.glb",
        "assets/models/flatgrass.glb",
        "assets/models/rock.glb",
    }
}
```

**Benefits**
- Preload declares assets before `setup()` runs, so all in-scene spawning hits the cache instantly
- Spawn location is config, not hardcoded Lua
- Scene list is discoverable without executing any Lua (useful for menus, scene selectors)

**Implementation notes**
- `load_lua_scene` would accept a scene config path instead of a script path directly
- Preloader warms the asset cache before the script runs
- Current refcount cache frees on last instance — need a "pinned" cache concept (or keep preloaded objects hidden) to hold entries alive across the scene lifetime
