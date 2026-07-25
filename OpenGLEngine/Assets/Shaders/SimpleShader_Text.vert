#version 410 core

uniform mat4 projection;

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texcoord;

out vec2 fragTexCoord;

void main()
{
    gl_Position = projection * vec4(position, 0.0, 1.0);
    fragTexCoord = texcoord;
}
