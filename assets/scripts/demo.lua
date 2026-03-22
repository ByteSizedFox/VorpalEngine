local show_notification = false
local e_pressed_last = false

function setup(scene)
    print("Lua Setup!")
    
    -- Load character (C++ manages mesh lifetime)
    local mesh = scene:create_object("assets/models/male_07.glb")
    mesh:setRotation(vec3.new(0.0, radians(0.0), radians(180.0)))
    mesh:setPosition(vec3.new(0.01, -2.0, 0.01))
    mesh:createRigidBody(50.0, ColliderType.CONVEXHULL)

    -- Load map
    local mesh1 = scene:create_object("assets/models/flatgrass.glb")
    mesh1:setPosition(vec3.new(0.0, -5.0, 0.0))
    mesh1:setRotation(vec3.new(0.0, 0.0, 0.0))
    mesh1:createRigidBody(0.0, ColliderType.TRIMESH)

    print("Lua Setup Finished!")
end

function draw(scene, window)
    -- Handle mouse interaction with UI (all logic in C++, triggered by Lua)
    scene:handleUIInteraction()

    -- Handle "e" hotkey with simple debounce
    local e_pressed = window:isKeyPressed(KEY_E)
    if e_pressed and not e_pressed_last then
        show_notification = not show_notification
        
        -- Teleport UI in front of player (mirroring MainScene logic)
        local camPos = scene.camera:getPosition()
        local forward = scene.camera:getForward()
        scene.uiMesh:setPosition(camPos - forward * 0.5)
        scene.uiMesh:setOrientation(scene.camera:getOrientation())
    end
    e_pressed_last = e_pressed

    -- Handle Escape key to exit
    if window:isKeyPressed(KEY_ESCAPE) then
        window:setShouldClose(true)
    end

    -- Default drawing behavior
    scene:draw(window)
end

function drawUI(scene, window)
    -- Fullscreen ImGui window relative to the UIMesh
    imgui.set_next_window_size(UI_WIDTH, UI_HEIGHT)
    imgui.set_next_window_pos(0, 0)
    
    imgui.begin("Lua UI")
    
    if show_notification then
        imgui.text("Hello from Lua!")
        imgui.text("Press 'E' to toggle and teleport this window.")
        if imgui.button("Close") then
            show_notification = false
        end
    else
        imgui.text("Press 'E' to show/teleport notification.")
    end

    if imgui.button("Switch to Scene 2") then
        scene:load_lua_scene("assets/scripts/scene2.lua")
    end

    if imgui.button("Reload This Scene") then
        scene:load_lua_scene("assets/scripts/demo.lua")
    end
    
    imgui.text("Press Escape to Exit")
    if imgui.button("Exit Game") then
        window:setShouldClose(true)
    end
    
    imgui.window_end()
end
