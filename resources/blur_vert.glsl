#version 330 core
layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec2 vertTex;

out vec2 TexCoords;

void main()
{
	TexCoords = vertTex;
	gl_Position = vec4(vertPos, 1.0);
}