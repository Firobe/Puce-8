#version 120
#
attribute vec2 in_Vertex;
attribute vec2 in_TexCoord0;
uniform mat4 modViewProj;
uniform vec2 screen;
varying vec2 texCoord;

void main(){
	vec4 vert = modViewProj * vec4(in_Vertex, 0., 1.);
	vec2 new = vec2(2 * vert.x / screen.x -1, 2 * vert.y / screen.y - 1);
	gl_Position = vec4(new, 0., 1.);
	texCoord = in_TexCoord0;
}
