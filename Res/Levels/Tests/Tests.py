import Junkpile
from Junkpile import vec2, vec4

e = componentManager.newEntity()
t = transformSystem.addComponent(e)
s = spriteSystem.addComponent(e, "TestGif.gif")

e = componentManager.newEntity()
t = transformSystem.addComponent(e)
c = cameraSystem.addComponentPerspective(e)
cameraSystem.setCameraActive(e)
t.m_position.z = -250.0