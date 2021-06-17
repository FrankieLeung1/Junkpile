import Junkpile
from Junkpile import vec2, vec4

gen = Junkpile.TextureGenerator()
gen.size(1490, 256)
#gen.clear(vec4(124.0 / 255.0, 162.0 / 255.0, 0.0, 1.0))
gen.clear(vec4( `RGBA:Colour:0.48, 0.64, 0.0, 1.0` ))
gen.line(vec2( 0.0, 0.0 ), vec2( 1.0, 0.0 ), vec4( 1.0, 1.0, 1.0, 1.0 ))
gen.text("GRASS", vec2( 0.02, 0.0 ), 30, vec4(`RGBA:Text Colour:1.0, 1.0, 1.0, 1.0`))