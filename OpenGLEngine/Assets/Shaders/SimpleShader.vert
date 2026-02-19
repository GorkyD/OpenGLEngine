#version 410 core

uniform UniformData
{
	mat4 world;
	mat4 view;
	mat4 projection;
};

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texcoord;
layout(location = 2) in vec3 normal;

out vec2 fragTexCoord;
out vec3 fragNormal;
out vec3 fragWorldPos;

void main()
{
	vec4 worldPos = world * vec4(position, 1.0);
	gl_Position = projection * view * worldPos;

	fragTexCoord = texcoord;
	fragNormal = mat3(transpose(inverse(world))) * normal;
	fragWorldPos = worldPos.xyz;
}
