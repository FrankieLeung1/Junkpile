import Junkpile
from Junkpile import vec2, vec4

gen = Junkpile.TextureGenerator()
gen.clear(vec4( `RGBA:Background:1.0, 0.0, 0.0, 1.0` ))
textScale = `float:TextScale:0.25`
print(textScale)
gen.text(`string:Label:"PROTOTYPE TEXTURE"`, vec2( 0.0, 0.0 ), 30.0, vec4(1.0, 1.0, 1.0, 1.0))

x = `float:x:0.5`
y = `float:y:0.5`
gen.line(vec2( x, 0.0 ), vec2( x, 1.0 ), vec4( 1.0, 1.0, 1.0, 1.0 ))
gen.line(vec2( 0.0, y ), vec2( 1.0, y ), vec4( 1.0, 1.0, 1.0, 1.0 ))

gen.line(vec2( 0.0, 0.25 ), vec2( 1.0, 0.25 ), vec4( 1.0, 1.0, 1.0, 0.25 ))
gen.line(vec2( 0.75, 0.0 ), vec2( 0.75, 1.0 ), vec4( 1.0, 1.0, 1.0, 0.25 ))
gen.line(vec2( 0.25, 0.0 ), vec2( 0.25, 1.0 ), vec4( 1.0, 1.0, 1.0, 0.25 ))
gen.line(vec2( 0.0, 0.75 ), vec2( 1.0, 0.75 ), vec4( 1.0, 1.0, 1.0, 0.25 ))