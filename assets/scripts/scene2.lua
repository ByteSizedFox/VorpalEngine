local show_notification = false
local e_pressed_last = false

function setup(scene)
    print("Scene 2 Setup!")
    
    -- Load character
    local mesh = scene:create_object("assets/models/male_07.glb")
    mesh:setRotation(vec3.new(0.0, radians(0.0), radians(180.0)))
    mesh:setPosition(vec3.new(0.01, -2.0, 0.01))
    mesh:createRigidBody(50.0, ColliderType.CONVEXHULL)

    -- Load floor instead of flatgrass
    local mesh1 = scene:create_object("assets/models/floor.glb")
    mesh1:setPosition(vec3.new(0.0, -5.0, 0.0))
    mesh1:setRotation(vec3.new(0.0, 0.0, 0.0))
    mesh1:createRigidBody(0.0, ColliderType.TRIMESH)

    print("Scene 2 Setup Finished!")
end

function draw(scene, window)
    scene:handleUIInteraction()

    local e_pressed = window:isKeyPressed(KEY_E)
    if e_pressed and not e_pressed_last then
        show_notification = not show_notification
        local camPos = scene.camera:getPosition()
        local forward = scene.camera:getForward()
        scene.uiMesh:setPosition(camPos - forward * 0.5)
        scene.uiMesh:setOrientation(scene.camera:getOrientation())
    end
    e_pressed_last = e_pressed

    if window:isKeyPressed(KEY_ESCAPE) then
        window:setShouldClose(true)
    end

    scene:draw(window)
end

function drawUI(scene, window)
    imgui.set_next_window_size(UI_WIDTH, UI_HEIGHT)
    imgui.set_next_window_pos(0, 0)
    
    imgui.begin("Scene 2 UI")
    
    imgui.text("This is Scene 2 (Using floor.glb)")
    
    if show_notification then
        imgui.text("Notification active!")
    end

    if imgui.button("Switch to Demo Scene") then
        scene:load_lua_scene("assets/scripts/demo.lua")
    end
    
    if imgui.button("Reload This Scene") then
        scene:load_lua_scene("assets/scripts/scene2.lua")
    end
    
    if imgui.button("Exit Game") then
        window:setShouldClose(true)
    end
    
    imgui.window_end()
end
