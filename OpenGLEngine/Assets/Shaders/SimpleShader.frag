#version 410 core

in vec2 fragTexCoord;
in vec3 fragNormal;
in vec3 fragWorldPos;

uniform sampler2D diffuseTexture;

out vec4 outColor;

void main()
{
	vec3 lightDir = normalize(vec3(0.5, 1.0, -0.3));
	vec3 norm = normalize(fragNormal);

	float diff = max(dot(norm, lightDir), 0.0);
	float ambient = 0.2;
	float lighting = ambient + diff * 0.8;

	vec4 texColor = texture(diffuseTexture, fragTexCoord);
	if (texColor.a < 0.01)
		texColor = vec4(1.0);

	outColor = vec4(texColor.rgb * lighting, 1.0);
}
