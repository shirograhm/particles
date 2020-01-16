#pragma once
#ifndef PARTICLE_H
#define PARTICLE_H

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

using ::glm::vec3;

class Particle {
public:
	float mass;
	vec3 position;
	vec3 velocity;
	vec3 forces;

	Particle(float m, vec3 x, vec3 v, vec3 f);
	virtual ~Particle();
	void addForce(vec3 forceIn);
	void update(float dt);
};

#endif // PARTICLE_H
