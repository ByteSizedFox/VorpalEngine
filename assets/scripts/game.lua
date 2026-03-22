-- Flatgrass Rescue Game
local survivors = {}
local rescued_count = 0
local total_survivors = 5
local game_won = false
local start_time = 0
local end_time = 0
local last_e_pressed = false

-- Helper to get distance between two vectors
function get_distance(v1, v2)
    return (v1 - v2):length()
end

function setup(scene)
    print("Rescue Mission Setup!")
    
    -- Set sun position for shadows and visibility
    set_light_pos(500, 500, 500)

    -- Load Map
    local map = scene:create_object("assets/models/flatgrass.glb")
    map:setPosition(vec3.new(0, -5, 0))
    map:createRigidBody(0, ColliderType.TRIMESH)

    -- Spawn Survivors at semi-random locations
    local locations = {
        vec3.new(20, -5, 20),
        vec3.new(-30, -5, 10),
        vec3.new(5, -5, -40),
        vec3.new(45, -5, -15),
        vec3.new(-15, -5, -25)
    }

    for i=1, total_survivors do
        local person = scene:create_object("assets/models/male_07.glb")
        local pos = locations[i]
        person:setPosition(pos)
        person:setRotation(vec3.new(0, radians(math.random(0, 360)), radians(180)))
        
        table.insert(survivors, {
            mesh = person,
            pos = pos,
            rescued = false
        })
    end

    start_time = os.clock()
    print("Game Ready!")
end

function draw(scene, window)
    scene:handleUIInteraction()

    local camPos = scene.camera:getPosition()

    -- Check for "Rescue" (Distance check)
    if not game_won then
        for i, s in ipairs(survivors) do
            if not s.rescued then
                local dist = get_distance(camPos, s.pos)
                if dist < 3.0 then
                    s.rescued = true
                    rescued_count = rescued_count + 1
                    -- "Hide" the rescued person
                    s.mesh:setPosition(vec3.new(0, -100, 0))
                    print("Rescued survivor " .. rescued_count)
                end
            end
        end

        if rescued_count == total_survivors then
            game_won = true
            end_time = os.clock()
        end
    end

    -- Teleport UI with E
    local e_pressed = window:isKeyPressed(KEY_E)
    if e_pressed and not last_e_pressed then
        local forward = scene.camera:getForward()
        scene.uiMesh:setPosition(camPos - forward * 0.5)
        scene.uiMesh:setOrientation(scene.camera:getOrientation())
    end
    last_e_pressed = e_pressed

    -- Exit with Escape
    if window:isKeyPressed(KEY_ESCAPE) then
        window:setShouldClose(true)
    end

    scene:draw(window)
end

function drawUI(scene, window)
    imgui.set_next_window_size(UI_WIDTH, UI_HEIGHT)
    imgui.set_next_window_pos(0, 0)
    
    imgui.begin("Rescue Radar")

    if not game_won then
        local current_time = os.clock() - start_time
        imgui.text("MISSION: RESCUE THE STRANDED")
        imgui.text("----------------------------")
        imgui.text("Survivors Found: " .. rescued_count .. " / " .. total_survivors)
        imgui.text(string.format("Time Elapsed: %.1f s", current_time))
        imgui.text("")
        imgui.text("HINT: Find people standing on the grass.")
        imgui.text("Press 'E' to bring this radar in front of you.")
    else
        imgui.text("MISSION ACCOMPLISHED!")
        imgui.text("----------------------------")
        imgui.text("All survivors rescued!")
        imgui.text(string.format("Final Time: %.2f seconds", end_time - start_time))
        imgui.text("")
        if imgui.button("Play Again") then
            scene:load_lua_scene("assets/scripts/game.lua")
        end
    end

    imgui.text("")
    if imgui.button("Quit to Desktop") then
        window:setShouldClose(true)
    end

    imgui.window_end()
end
