#include "../headers/Particle.h"

Particle::Particle(float m, vec3 x, vec3 v, vec3 f) {
	mass = m;
	position = x;
	velocity = v;
	forces = f;
}

Particle::~Particle() {

}

void Particle::addForce(vec3 forceIn) {
	forces.x += forceIn.x;
	forces.y += forceIn.y;
	forces.z += forceIn.z;
}

void Particle::update(float dt) {
	// Get acceleration for velocity calculation
	vec3 accel = forces / mass;

	velocity += accel * dt;
	position += velocity * dt;

	forces = vec3(0);
}