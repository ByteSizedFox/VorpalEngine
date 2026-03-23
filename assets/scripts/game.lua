-- HUNTED: A VorpalEngine Survival Game
-- Survive as long as possible against waves of zombies.
-- Each zombie that spawns is faster than the last. Use rocks for cover.

-- Session high score persists across restarts if Lua state is not fully reset
if not _G._hunted_hs then _G._hunted_hs = 0 end

local start_time = 0
local score_time = 0
local game_over = false

local zombies = {}
local zombie_count = 0
local spawn_timer = 0
local new_spawn_flash = 0.0

-- Tuning
local ZOMBIE_BASE_SPEED  = 2.8
local ZOMBIE_SPEED_INC   = 0.5
local MAX_ZOMBIES        = 8
local SPAWN_INTERVAL     = 20.0
local DETECTION_RANGE    = 38.0
local CATCH_RANGE        = 1.8
local MAP_EDGE           = 58.0
local ZOMBIE_SCALE       = 0.01   -- Mixamo GLB geometry is in cm

-- Capsule dims — must match createCapsuleRigidBody call below
local CAPSULE_RADIUS     = 0.3
local CAPSULE_HEIGHT     = 1.0
-- Distance from capsule centre to the bottom; used to place visual root at feet level
local CAPSULE_FOOT_OFFSET = CAPSULE_RADIUS + CAPSULE_HEIGHT * 0.5

-- UI state: updated each draw(), read in drawUI()
local ui_nearest_dist  = 999.0
local ui_nearest_angle = 0.0   -- degrees relative to camera forward
local ui_spawn_timer   = 0.0

local last_e_pressed = false

math.randomseed(os.time())

-- ---------------------------------------------------------------------------
-- Helpers
-- ---------------------------------------------------------------------------

local function rand_patrol_pos()
    local a = math.random() * math.pi * 2
    local r = 8 + math.random() * 35
    return vec3.new(math.cos(a) * r, 0, math.sin(a) * r)
end

local function edge_spawn_pos()
    local a = math.random() * math.pi * 2
    local r = MAP_EDGE + 4 + math.random() * 8
    -- Spawn above ground so gravity drops them onto the surface
    return vec3.new(math.cos(a) * r, 2, math.sin(a) * r)
end

local function dist2d(a, b)
    local dx = a.x - b.x
    local dz = a.z - b.z
    return math.sqrt(dx * dx + dz * dz)
end

local function spawn_zombie(scene)
    zombie_count = zombie_count + 1
    local speed = ZOMBIE_BASE_SPEED + (zombie_count - 1) * ZOMBIE_SPEED_INC
    local pos   = edge_spawn_pos()

    local mesh = scene:create_skinned_object("assets/models/zombie.glb")
    mesh:setPosition(pos)
    mesh:setScale(ZOMBIE_SCALE, ZOMBIE_SCALE, ZOMBIE_SCALE)
    mesh:playAnimation(0)
    mesh:setLooping(true)
    mesh:setAnimSpeed(0.7 + (zombie_count - 1) * 0.1)
    mesh:createCapsuleRigidBody(70.0, CAPSULE_RADIUS, CAPSULE_HEIGHT)
    mesh:setFriction(0.0)
    mesh:setDamping(0.0, 0.0)
    scene:registerPhysics(mesh)

    table.insert(zombies, {
        mesh          = mesh,
        speed         = speed,
        patrol_target = rand_patrol_pos(),
    })

    new_spawn_flash = 3.0
    print(string.format("[HUNTED] Zombie %d spawned  speed=%.1f", zombie_count, speed))
end

-- ---------------------------------------------------------------------------
-- Setup
-- ---------------------------------------------------------------------------

function setup(scene)
    start_time  = get_time()
    spawn_timer = 0

    -- Static floor with trimesh collision
    local floor = scene:create_object("assets/models/flatgrass.glb")
    floor:setPosition(vec3.new(0, -5, 0))
    floor:createRigidBody(0, ColliderType.TRIMESH)

    -- Scatter rocks for cover.
    -- ROCK_SCALE: if rocks appear too large/small, adjust this.
    local ROCK_SCALE = 1.0
    local rock_spots = {
        { 12,  8,  0.9}, {-14, 10,  1.2}, { 20, -12, 0.8}, { -8, -20, 1.3},
        { 26,  4,  1.0}, {-22,  -6, 0.9}, {  6,  24, 1.1}, {-16,  22, 0.8},
        { 16, -25, 1.2}, {-28, 14,  1.0}, { 32,  -5, 0.9}, {-32, -10, 1.1},
        { 10, -32, 0.8}, {-18, -28, 1.3}, { 28,  20, 1.0}, { -6,  35, 0.9},
    }
    for _, rp in ipairs(rock_spots) do
        local rock = scene:create_object("assets/models/rock.glb")
        local s = ROCK_SCALE * rp[3]
        rock:setPosition(vec3.new(rp[1], -5, rp[2]))
        rock:setScale(s, s, s)
        rock:createRigidBody(0, ColliderType.CONVEXHULL)
    end

    -- First zombie spawns immediately so there is instant threat
    spawn_zombie(scene)

    set_light_pos(400, 600, 200)
    print("[HUNTED] Setup complete.")
end

-- ---------------------------------------------------------------------------
-- Draw (game logic + rendering)
-- ---------------------------------------------------------------------------

function draw(scene, window)
    scene:handleUIInteraction()

    local dt  = get_delta_time()
    local now = get_time()

    -- E: pull the UI panel to the player
    local e_pressed = window:isKeyPressed(KEY_E)
    if e_pressed and not last_e_pressed then
        local cp  = scene.camera:getPosition()
        local fwd = scene.camera:getForward()
        scene.uiMesh:setPosition(cp - fwd * 0.5)
        scene.uiMesh:setOrientation(scene.camera:getOrientation())
    end
    last_e_pressed = e_pressed

    if window:isKeyPressed(KEY_ESCAPE) then
        window:setShouldClose(true)
    end

    if not game_over then
        score_time = now - start_time

        local camPos = scene.camera:getPosition()
        local camFwd = scene.camera:getForward()

        -- Engine getForward() points backward; negate for actual view direction in XZ
        local fx = -camFwd.x
        local fz = -camFwd.z
        local fl = math.sqrt(fx * fx + fz * fz)
        if fl > 0.001 then fx = fx / fl; fz = fz / fl end

        -- Zombie spawning
        spawn_timer = spawn_timer + dt
        if new_spawn_flash > 0 then new_spawn_flash = new_spawn_flash - dt end

        if spawn_timer >= SPAWN_INTERVAL and zombie_count < MAX_ZOMBIES then
            spawn_zombie(scene)
            spawn_timer = 0
        end

        ui_nearest_dist = 999.0
        ui_spawn_timer  = spawn_timer

        -- Update each zombie
        for _, z in ipairs(zombies) do
            -- Sync visual mesh to physics capsule: place model root at capsule bottom
            local physPos = z.mesh:getPhysicsPosition()
            z.mesh:setPosition(vec3.new(physPos.x, physPos.y - CAPSULE_FOOT_OFFSET, physPos.z))

            local zpos = z.mesh:getPosition()
            local d    = dist2d(camPos, zpos)

            -- Track nearest zombie for UI radar
            if d < ui_nearest_dist then
                ui_nearest_dist = d
                -- Signed angle from camera-forward to zombie direction
                local tx = zpos.x - camPos.x
                local tz = zpos.z - camPos.z
                local tl = math.sqrt(tx * tx + tz * tz)
                if tl > 0.001 then tx = tx / tl; tz = tz / tl end
                local dot   =  fx * tx + fz * tz
                local cross =  fx * tz - fz * tx
                ui_nearest_angle = math.atan(cross, dot) * 180.0 / math.pi
            end

            -- Caught!
            if d < CATCH_RANGE then
                game_over = true
                if score_time > _G._hunted_hs then
                    _G._hunted_hs = score_time
                end
                break
            end

            -- Choose movement target: chase or patrol
            local target
            if d < DETECTION_RANGE then
                target = camPos
            else
                if dist2d(zpos, z.patrol_target) < 2.0 then
                    z.patrol_target = rand_patrol_pos()
                end
                target = z.patrol_target
            end

            -- Drive horizontal velocity; preserve vertical (gravity) from physics
            local mdx = target.x - zpos.x
            local mdz = target.z - zpos.z
            local mlen = math.sqrt(mdx * mdx + mdz * mdz)
            if mlen > 0.1 then
                mdx = mdx / mlen
                mdz = mdz / mlen
                local curVel = z.mesh:getLinearVelocity()
                z.mesh:setLinearVelocity(vec3.new(mdx * z.speed, curVel.y, mdz * z.speed))

                -- Rotate zombie to face direction of travel.
                -- If zombies face the wrong way, add math.pi to the angle below.
                local angle = math.atan(mdx, mdz)
                z.mesh:setRotation(vec3.new(0, angle, 0))
            end
        end
    end

    scene:draw(window)
end

-- ---------------------------------------------------------------------------
-- DrawUI (HUD)
-- ---------------------------------------------------------------------------

local function bearing_label(deg)
    if     deg > -45  and deg <=  45  then return "^ AHEAD"
    elseif deg >  45  and deg <= 135  then return "> RIGHT"
    elseif deg > -135 and deg <= -45  then return "< LEFT"
    else                                   return "v BEHIND" end
end

function drawUI(scene, window)
    imgui.set_next_window_size(UI_WIDTH, UI_HEIGHT)
    imgui.set_next_window_pos(0, 0)
    imgui.begin("HUNTED")

    if not game_over then
        imgui.text(string.format("Time:     %.1f s", score_time))
        imgui.text(string.format("Threats:  %d / %d", zombie_count, MAX_ZOMBIES))
        if _G._hunted_hs > 0 then
            imgui.text(string.format("Best:     %.1f s", _G._hunted_hs))
        end

        imgui.separator()

        -- Danger display with colored text
        if ui_nearest_dist < 5 then
            imgui.text_colored(1, 0, 0, 1, "!! CRITICAL !!")
            imgui.progress_bar(1.0, "CRITICAL")
        elseif ui_nearest_dist < 10 then
            imgui.text_colored(1, 0.3, 0, 1, "! DANGER")
            imgui.progress_bar(clamp(1 - (ui_nearest_dist - 5) / 5, 0, 1), "DANGER")
        elseif ui_nearest_dist < 20 then
            imgui.text_colored(1, 0.8, 0, 1, "~ WARNING")
            imgui.progress_bar(clamp(1 - (ui_nearest_dist - 10) / 10, 0, 1), "CLOSE")
        elseif ui_nearest_dist < DETECTION_RANGE then
            imgui.text_colored(0.8, 0.8, 0, 1, ". NEARBY")
            imgui.progress_bar(clamp(1 - (ui_nearest_dist - 20) / 18, 0, 1), "NEARBY")
        else
            imgui.text_colored(0, 1, 0, 1, "CLEAR")
            imgui.progress_bar(0.0, "CLEAR")
        end

        if ui_nearest_dist < 999 then
            imgui.text(string.format("  %.0f m  %s", ui_nearest_dist, bearing_label(ui_nearest_angle)))
        end

        imgui.separator()

        if new_spawn_flash > 0 then
            imgui.text_colored(1, 0.4, 0, 1, "!! NEW THREAT SPAWNING !!")
        elseif zombie_count >= MAX_ZOMBIES then
            imgui.text("All threats active.")
        else
            imgui.text(string.format("Next in: %.0f s", SPAWN_INTERVAL - ui_spawn_timer))
        end

        imgui.separator()
        imgui.text("WASD + Mouse | SHIFT sprint")
        imgui.text("SPACE jump   | E = this menu")
    else
        imgui.text_colored(1, 0, 0, 1, "CAUGHT.")
        imgui.separator()
        imgui.text(string.format("Survived:  %.1f s", score_time))
        imgui.text(string.format("Threats:   %d spawned", zombie_count))
        imgui.separator()
        if score_time >= _G._hunted_hs then
            imgui.text_colored(1, 1, 0, 1, "[ NEW BEST TIME ]")
        else
            imgui.text(string.format("Best: %.1f s", _G._hunted_hs))
        end
        imgui.spacing()
        if imgui.button("Try Again") then
            scene:load_lua_scene("assets/scripts/game.lua")
        end
    end

    imgui.separator()
    if imgui.button("Quit") then
        window:setShouldClose(true)
    end

    imgui.window_end()
end
