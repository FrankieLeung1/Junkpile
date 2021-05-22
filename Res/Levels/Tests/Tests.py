import Junkpile
from Junkpile import vec2, vec4
import random

def print(s):
    pass

#cloud
print("cloud")
e = componentManager.newEntity()
t = transformSystem.addComponent(e)
t.m_position.x = -450.0
t.m_position.y = 250.0
s = spriteSystem.addComponent(e, "Art/Background Elements/Flat/cloud1.png")

#floor grid
print("floor grid")
e = componentManager.newEntity()
t = transformSystem.addComponent(e)
t.m_position.x = 0.0
t.m_position.y = -390.0
s = spriteSystem.addComponent(e, "Scripts/Generators/Floor.py")

# character
print("character")
character = componentManager.newEntity()
t = transformSystem.addComponent(character)
t.m_position.x = 0.0
s = spriteSystem.addComponent(character, "TestGif.gif")
p = physicsSystem.createBox(character, Junkpile.vec3(100, 100, 1000), 1)

# coins
coinPositions = [[-400.0, 90.0], [500.0, 90.0]]
coins = []
def spawnCoins(pos, mass):
    coin = componentManager.newEntity()
    t = transformSystem.addComponent(coin)
    t.m_position.x = pos[0]
    t.m_position.y = pos[1]
    t.m_scale.x = 0.4
    t.m_scale.y = 0.4
    p = physicsSystem.createBox(coin, Junkpile.vec3(40, 40, 40), mass)
    s = spriteSystem.addComponent(coin, "doge.png")
    coins.append(coin)

print("coins1")
spawnCoins(coinPositions[0], 0)
print("coins2")
spawnCoins(coinPositions[1], 0)

#floor
print("floor")
e = componentManager.newEntity()
t = transformSystem.addComponent(e)
t.m_position.x = 0.0
t.m_position.y = -250.0
p = physicsSystem.createBox(e, Junkpile.vec3(10000, 1, 10000), 0)

#camera
print("camera")
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
    elif inputManager.justDown(82): # "R"
        spawnCoins(coinPositions[0], 0)
        spawnCoins(coinPositions[1], 0)
    elif inputManager.justDown(69): # "E"
        spawnCoins([random.uniform(-20.0, 20.0), 300.0], 1)

print("inputChanged")
eventManager.addListener_InputChanged(inputChanged, 0)

def onCollision(e):
    other = None

    if e.getEntity1().equals(character):
        other = e.getEntity2()
    elif e.getEntity2().equals(character):
        other = e.getEntity1()
    
    if other is None:
        return

    for coin in coins:
        if other.equals(coin):
            componentManager.removeEntity(coin)
            coins.remove(coin)

print("onCollision")
eventManager.addListener_CollisionEvent(onCollision, 0)


def update(e):
    ImGui.Begin("Test")
    ImGui.End()
#eventManager.addListener_UpdateEvent(update, 0)