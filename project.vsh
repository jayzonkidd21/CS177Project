#version 120

attribute vec2 position;
attribute vec4 color;

uniform mat3 mvpMatrix;
uniform float t;

varying vec4 out_color;
varying vec2 pos;

void main() 
{
	pos = ( mvpMatrix * vec3( position, 1 ) ).xy;
	out_color = 0.3 + color * sin( t );
	gl_Position= vec4( pos, 0.0, 1.0);
}