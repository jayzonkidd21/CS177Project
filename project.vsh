#version 120

attribute vec2 position;
attribute vec4 color;

uniform mat3 mvpMatrix;

varying vec4 out_color;
varying vec2 pos;

void main() 
{
	pos = ( mvpMatrix * vec3( position, 1 ) ).xy;
	out_color = color;
	gl_Position= vec4( pos, 0.0, 1.0);
}