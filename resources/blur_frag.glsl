#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D image;

uniform bool horizontal;
uniform float weight[5] = float[](0.222928, 0.181732, 0.150598, 0.125977, 0.098229);

void main()
{
	// Get size of single texel
	vec2 tex_offset = 1.0 / textureSize(image, 0);
	// Current fragment blur contribution
	vec3 result = texture(image, TexCoords).rgb * weight[0];

	// Blur time!
	if (horizontal) {
		for (int i = 1; i < 5; ++i) {
			result += texture(image, TexCoords + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
			result += texture(image, TexCoords - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
		}
	} else {
		for (int i = 1; i < 5; ++i) {
			result += texture(image, TexCoords + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
			result += texture(image, TexCoords - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
		}
	}
	// Output result
	FragColor = vec4(result, 1.0);
}