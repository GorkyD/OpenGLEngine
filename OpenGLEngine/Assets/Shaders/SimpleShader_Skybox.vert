#version 410 core

uniform UniformData
{
    mat4 world;
    mat4 view;
    mat4 projection;
};

layout(location = 0) in vec3 position;

out vec3 texCoord;

void main()
{
    texCoord = position;

    // Strip translation so the skybox stays centered on the camera.
    mat4 viewNoTranslation = mat4(mat3(view));
    vec4 clipPos = projection * viewNoTranslation * vec4(position, 1.0);

    // Force depth to the far plane (z / w = 1.0) so the skybox is drawn behind everything else.
    gl_Position = clipPos.xyww;
}
