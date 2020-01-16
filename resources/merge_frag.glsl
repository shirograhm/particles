#version 330 core
in vec2 TexCoords;

uniform sampler2D scene;
uniform sampler2D bloomBlur;

out vec4 color;

void main()
{
	vec3 normalSceneColor = texture(scene, TexCoords).rgb;
	vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;

	vec3 result = normalSceneColor + bloomColor;
	color = vec4(result, 1.0);
}