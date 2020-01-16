#version 330 core
in vec3 fragNormal;
in vec3 fragPos;

layout(location = 0) out vec4 color;

void main() {
	color = vec4(0.75, 0.72, 0.69, 1.0);
}
