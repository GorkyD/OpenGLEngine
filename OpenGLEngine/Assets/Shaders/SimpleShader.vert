#version 410 core

uniform UniformData
{
	mat4 world;
	mat4 view;
	mat4 projection;
};


layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texcoord;

layout(location = 0) out vec3 vertOutColor;

void main()
{
	vec4 pos = projection * view * world * vec4(position,1);

	gl_Position = pos;
	vertOutColor = vec3(texcoord.x, texcoord.y,0);
}