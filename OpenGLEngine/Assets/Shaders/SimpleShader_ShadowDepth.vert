#version 410 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texcoord;

uniform UniformData
{
    mat4 world;
    mat4 view;
    mat4 projection;
};
uniform mat4 lightSpaceMatrix;

out vec2 fragTexCoord;

void main()
{
    gl_Position = lightSpaceMatrix * world * vec4(position, 1.0);
    fragTexCoord = texcoord;
}
