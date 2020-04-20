import Junkpile
from Junkpile import vec2, vec4

gen = Junkpile.TextureGenerator()
gen.clear(vec4( 1.0, 0.0, 0.0, 1.0 ))
gen.text("PROTOTYPE TEXTURE", vec2( 0.0, 0.0 ), 10, vec4(1.0, 1.0, 1.0, 1.0))
gen.line(vec2( 0.5, 0.0 ), vec2( 0.5, 1.0 ), vec4( 1.0, 1.0, 1.0, 1.0 ))
gen.line(vec2( 0.0, 0.5 ), vec2( 1.0, 0.5 ), vec4( 1.0, 1.0, 1.0, 1.0 ))
gen.line(vec2( 0.0, 0.25 ), vec2( 1.0, 0.25 ), vec4( 1.0, 1.0, 1.0, 0.25 ))
gen.line(vec2( 0.75, 0.0 ), vec2( 0.75, 1.0 ), vec4( 1.0, 1.0, 1.0, 0.25 ))
gen.line(vec2( 0.25, 0.0 ), vec2( 0.25, 1.0 ), vec4( 1.0, 1.0, 1.0, 0.25 ))
gen.line(vec2( 0.0, 0.75 ), vec2( 1.0, 0.75 ), vec4( 1.0, 1.0, 1.0, 0.25 ))