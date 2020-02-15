-- TODO: enforce no globals
-- TODO: require()

local game = require("game")
local player = {}

local actor = Components:addActorComponent()
local sprite = Components:addSpriteComponent()
local collision = sprite:addCollisionComponent()

function player.onKeyPress(key)

end
Events:addKeyPressListener(player.onKeyPress)

function player.onCollision(other)

end
Events:addCollisionListener(player.onCollision)

return player