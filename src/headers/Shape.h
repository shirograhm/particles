#pragma once
#ifndef LAB471_SHAPE_H_INCLUDED
#define LAB471_SHAPE_H_INCLUDED

#include <string>
#include <vector>
#include <memory>
#include "../headers/Program.h"
#include <glm/gtc/type_ptr.hpp>
#include "tiny_obj_loader.h"

using ::glm::vec3;
using ::std::vector;
using ::tinyobj::shape_t;

class Shape
{
public:
	void createShape(shape_t& shape);
	void init();
	void measure();
	void draw(const Program *prog) const;

	vec3 min = vec3(0);
	vec3 max = vec3(0);

private:
	vector<unsigned int> eleBuf;
	vector<float> posBuf;
	vector<float> norBuf;
	vector<float> texBuf;

	unsigned int eleBufID = 0;
	unsigned int posBufID = 0;
	unsigned int norBufID = 0;
	unsigned int texBufID = 0;
	unsigned int vaoID = 0;
};

#endif // LAB471_SHAPE_H_INCLUDED
