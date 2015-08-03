#version 430 core

uniform mat4 mvp;
uniform mat4 inv_mv;
uniform mat4 mv;
uniform mat4 normal_mat;
uniform vec3 ll, ur, cam_pos;

layout(location=0) in vec4 in_vertex;

void main()
{
  gl_Position = mvp * in_vertex;
}
