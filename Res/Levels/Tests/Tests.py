import Junkpile
from Junkpile import vec2, vec4

#cloud
e = componentManager.newEntity()
t = transformSystem.addComponent(e)
t.m_position.x = -450.0
t.m_position.y = 250.0
s = spriteSystem.addComponent(e, "Art/Background Elements/Flat/cloud1.png")

#floor grid
e = componentManager.newEntity()
t = transformSystem.addComponent(e)
t.m_position.x = 0.0
t.m_position.y = -390.0
s = spriteSystem.addComponent(e, "Scripts/Generators/Floor.py")

# character
character = componentManager.newEntity()
t = transformSystem.addComponent(character)
t.m_position.x = 0.0
s = spriteSystem.addComponent(character, "TestGif.gif")
p = physicsSystem.createBox(character, Junkpile.vec3(100, 100, 1000), 1)

# coins
coinPositions = [[-400.0, 0.0], [-200.0, 0.0], [500.0, 0.0]]
for pos in coinPositions:
    e = componentManager.newEntity()
    t = transformSystem.addComponent(e)
    t.m_position.x = pos[0]
    t.m_position.y = pos[1]
    t.m_scale.x = 0.6
    t.m_scale.y = 0.6
    s = spriteSystem.addComponent(e, "Art/Puzzle Assets 2/Coins/coin_01.png")

#floor
e = componentManager.newEntity()
t = transformSystem.addComponent(e)
t.m_position.x = 0.0
t.m_position.y = -250.0
p = physicsSystem.createBox(e, Junkpile.vec3(10000, 1, 10000), 0)

#camera
e = componentManager.newEntity()
t = transformSystem.addComponent(e)
c = cameraSystem.addComponentPerspective(e)
cameraSystem.setCameraActive(e)
#cameraSystem.setWASDInput(e)
t.m_position.z = -250.0

def inputChanged(e):
    if inputManager.justDown(32):
        physicsSystem.impulse(character, Junkpile.vec3(0, 50, 0))
    elif inputManager.justDown(65):
        physicsSystem.impulse(character, Junkpile.vec3(-25, 0, 0))
    elif inputManager.justDown(68):
        physicsSystem.impulse(character, Junkpile.vec3(25, 0, 0))

eventManager.addListener_InputChanged(inputChanged, 0)

#imgui.Begin("Test")
#imgui.End()

def update(e):
    ImGui.Begin("Test")
    ImGui.End()
#eventManager.addListener_UpdateEvent(update, 0)