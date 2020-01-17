#version 330 core
in vec3 fragNormal;
in vec3 fragPosition;
in vec2 fragTexture;

uniform sampler2D globeTexture;

uniform vec3 lights[500];
uniform vec3 shapeColor;
uniform float shininess;
uniform bool isLightSource;
uniform bool isGlobeSphere;

layout(location = 0) out vec4 color;
layout(location = 1) out vec4 brights;

// Fixed light color
vec3 lightColor = vec3(0.85, 0.80, 0.75);

vec3 calculatePointLight(vec3 lightPosition, vec3 shapeColorIn) {
	// Fixed camera position
	vec3 camPosition = vec3(0);

	vec3 N = normalize(fragNormal);
	vec3 L = normalize(lightPosition - fragPosition);
	vec3 V = normalize(camPosition - fragPosition);
	vec3 R = reflect(-L, N);
	// attenuation
	float distance = length(lightPosition - fragPosition);
	float attenuation = 1.0f / (distance * distance);
	// combine results
	vec3 ambient = 0.05f * lightColor;
	vec3 diffuse = max(dot(N, L), 0) * lightColor;
	vec3 specular = pow(max(dot(V, R), 0), shininess) * lightColor;

	return (ambient + diffuse + specular) * attenuation * shapeColorIn;
}

void main() {
	// Output light color if its a firefly
	if (isLightSource) {
		color = vec4(lightColor, 1.0);
	} else {
		// Initialization of variables
		vec3 result = vec3(0);

		// Summation calculation for multiple lights
		for (int i = 0; i < lights.length(); i++) {
			vec3 texSample = texture(globeTexture, fragTexture).rgb;
			result += calculatePointLight(lights[i], isGlobeSphere ? texSample : shapeColor);
		}
		result /= lights.length();

		// Output normal color to first attachment if not light (layout = 0)
		color = vec4(result, 1.0);
	}

	// Bloom calculation for second attachment for both light and normal geometry (layout = 1)
	float brightnessThreshold = dot(color.rgb, vec3(0.3126, 0.7152, 0.1722));
	if (brightnessThreshold > 0.90f) {
		brights = vec4(color.rgb, 1.0f);
	}
	else {
		brights = vec4(vec3(0), 1.0f);
	}
}
