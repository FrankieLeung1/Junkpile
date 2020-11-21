import Junkpile
from Junkpile import vec2, vec4

#cloud
e = componentManager.newEntity()
t = transformSystem.addComponent(e)
t.m_position.x = -450.0
t.m_position.y = 250.0
s = spriteSystem.addComponent(e, "Art/Background Elements/Flat/cloud1.png")
#s = spriteSystem.addComponent(e, "Art/Donuts/donut_1.png")

# character
for i in range(1):
    character = componentManager.newEntity()
    t = transformSystem.addComponent(character)
    t.m_position.x = i * 10.0
    s = spriteSystem.addComponent(character, "TestGif.gif")
    p = physicsSystem.createBox(character, Junkpile.vec3(100, 100, 1000), 1)

#floor
e = componentManager.newEntity()
t = transformSystem.addComponent(e)
t.m_position.x = 0.0
t.m_position.y = -250.0
p = physicsSystem.createBox(e, Junkpile.vec3(99999, 1, 99999), 0)

#camera
e = componentManager.newEntity()
t = transformSystem.addComponent(e)
c = cameraSystem.addComponentPerspective(e)
cameraSystem.setCameraActive(e)
t.m_position.z = -250.0

def inputChanged(e):
    if inputManager.justDown(32):
        physicsSystem.impulse(character, Junkpile.vec3(0, 50, 0))
    elif inputManager.justDown(65):
        physicsSystem.impulse(character, Junkpile.vec3(-25, 0, 0))
    elif inputManager.justDown(68):
        physicsSystem.impulse(character, Junkpile.vec3(25, 0, 0))

eventManager.addListener_InputChanged(inputChanged, 0)

