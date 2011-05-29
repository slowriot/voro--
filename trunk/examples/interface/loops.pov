#version 3.6;
#include "colors.inc"
#include "metals.inc"
#include "textures.inc"

global_settings {
	max_trace_level 64
}

camera {
	location <2,30,-40>
	right 0.155*x*image_width/image_height
	up 0.155*y
	look_at <0,0,0>
}

background{rgb 1}

light_source{<-8,30,-20> color rgb <0.79,0.75,0.75>}
light_source{<25,12,-12> color rgb <0.36,0.40,0.40>}

#declare r=0.02;
#declare s=0.2;

union{
#include "loop1_m.pov"
	pigment{rgb <0.9,0.6,0.3>} finish{reflection 0.185 specular 0.3 ambient 0.42}
}

union{
#include "loop1_v.pov"
	pigment{rgb <0.9,0.4,0.45>} finish{specular 0.5 ambient 0.42}
}

union{
#include "loop2_m.pov"
	pigment{rgb <0.5,0.65,0.75>} finish{reflection 0.185 specular 0.3 ambient 0.42}
}

union{
#include "loop2_v.pov"
	pigment{rgb <0.42,0.42,0.75>} finish{specular 0.5 ambient 0.42}
}
