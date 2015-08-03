#version 430 core

uniform mat4 mvp, mv;

layout(location=0) in vec4 in_vertex;
layout(location=1) in vec2 in_texture;

out vec4 vs_pos;
out vec2 tex_coord;

void main()
{
	vs_pos = mv * in_vertex;
	tex_coord = in_texture;
  gl_Position = mvp * in_vertex;
}
