import Junkpile
from Junkpile import vec2, vec4

#cloud
#print("cloud")
e = componentManager.newEntity()
t = transformSystem.addComponent(e)
t.m_position.x = -450.0
t.m_position.y = 250.0
#t.m_scale.x = 0.4
#t.m_scale.y = 0.4
s = spriteSystem.addComponent(e, "Art/Background Elements/Flat/cloud2.png")