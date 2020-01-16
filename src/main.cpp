/* Xavier Graham
 * Base code for ChemDraw.io
 */

#include <iostream>
#include <algorithm>
#include <glad/glad.h>

#include "headers/GLSL.h"
#include "headers/Program.h"
#include "headers/MatrixStack.h"
#include "headers/Shape.h"
#include "headers/Texture.h"
#include "headers/Particle.h"
#include "headers/WindowManager.h"

// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define NUMBER_OF_FIREFLIES 500
#define FIREFLIES_PER_CLICK 10

class Application : public EventCallbacks
{
public:
	WindowManager* windowManager = nullptr;
	// Shader programs
	Program* blurBloomShader;
	Program* sceneShader;
	Program* finalShader;
	// Shapes
	vector<Shape*> sphere;
	vec3 sphereOffset;
	vector<Shape*> table;
	vec3 tableOffset;
	float tableScale = 1.5f;
	vector<Shape*> globe;
	vec3 globeOffset;
	float globeScale = 0.0025f;
	// Existing fireflies
	vector<Particle*> fireflies;
	// Existing magnets
	vector<Particle*> magnets;

	// Framebuffer for bloom
	GLuint bloomFBO;
	// Texture buffers for bloom
	GLuint bloomColorBuffers[2];
	// Depth render buffer
	unsigned int rboDepth;
	// FBO for pingpong blur
	unsigned int pingPongFBO[2];
	// Texture buffers for pingpong blurring
	unsigned int pingPongTextures[2];
	// Blur direction
	bool horizontal = true;

	// Center point
	vec3 centerPoint = vec3(0, 0, -2);

	// Toggles
	bool isGravityOn = false;
	bool isMagnetModeOn = false;
	bool isCenterPointAttractive = false;

	void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		using ::std::cerr;
		using ::std::endl;

		switch(key) {
			case GLFW_KEY_ESCAPE:
				glfwSetWindowShouldClose(window, (action == GLFW_PRESS));
				break;
			case GLFW_KEY_Z:
				glPolygonMode(GL_FRONT_AND_BACK, (action == GLFW_RELEASE) ? GL_FILL : GL_LINE);
				break;
			case GLFW_KEY_G:
				// Toggle gravity only on release (or press, just not both)
				if (action == GLFW_RELEASE) {
					isGravityOn = !isGravityOn;
				}
				break;
			case GLFW_KEY_M:
				// Toggle magnet only on release (or press, just not both)
				if (action == GLFW_RELEASE) {
					isMagnetModeOn = !isMagnetModeOn;
				}
				break;
			case GLFW_KEY_TAB:
				// Clear everything but scene
				magnets.clear();
				fireflies.clear();
				break;
			case GLFW_KEY_C:
				// Toggle center point attraction
				if (action == GLFW_RELEASE) {
					isCenterPointAttractive = !isCenterPointAttractive;
				}
			default:
				cerr << "This key is not associated with any program control." << endl;
		}
	}

	void scrollCallback(GLFWwindow* window, double deltaX, double deltaY)
	{
	}

	void mouseCallback(GLFWwindow* window, int button, int action, int mods)
	{
		using ::std::cout;
		using ::std::endl;

		int width, height;
		double posX, posY;

		if (action == GLFW_PRESS) {
			glfwGetCursorPos(window, &posX, &posY);
			glfwGetFramebufferSize(window, &width, &height);

			cout << "Cursor: (" << posX << ", " << posY << ")" << endl;

			float worldSpaceX = 4.0f * posX / width - 2;
			float worldSpaceY = -2.0f * posY / height + 1.0f;

			if (!isMagnetModeOn) {
				for (int i = 0; i < FIREFLIES_PER_CLICK; i++) {
					vec3 randVelo = vec3(generateRandomFloat(-0.15f, 0.15f), generateRandomFloat(-0.15f, 0.15f), generateRandomFloat(-0.15f, 0.15f));
					fireflies.push_back(new Particle(generateRandomFloat(0.7f, 1.2f), vec3(worldSpaceX, worldSpaceY, -2), randVelo, vec3(0, 0, 0)));
				}
			}
			else {
				magnets.push_back(new Particle(2.0f, vec3(worldSpaceX, worldSpaceY, generateRandomFloat(-2.5, -1.5)), vec3(0), vec3(0)));
			}
		}
	}

	void cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
	{

	}

	void resizeCallback(GLFWwindow* window, int width, int height)
	{
		glViewport(0, 0, width, height);
	}

	//code to set up the two shaders - a diffuse shader and texture mapping
	void init(const std::string& resourceDirectory)
	{
		// Check GLSL version
		GLSL::checkVersion();

		// Set background color
		glClearColor(.01f, .01f, .01f, 1.0f);
		// Enable z-buffer test
		glEnable(GL_DEPTH_TEST);

		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);

		// Initialize peripherals
		initializeShaderPrograms(resourceDirectory);
		initializeGeometry(resourceDirectory);

		// Create bloomFBO and color attachments
		initializeBloomFBOs(width, height);
		// Create FBO for ping pong blurring of bloom
		initializePingPongFBOs(width, height);
	}

	void initializeBloomFBOs(int width, int height) {
		// Initialize bloom framebuffer (1 buffer, 2 colorbuffers for it)
		glGenFramebuffers(1, &bloomFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, bloomFBO);
		// Initialize color buffers for our bloomFBO
		glGenTextures(2, bloomColorBuffers);

		// Bind texture for configuration
		for (int i = 0; i < 2; i++) {
			glBindTexture(GL_TEXTURE_2D, bloomColorBuffers[i]);
			// Give an empty image to OpenGL (the NULL term)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
			// Texture and color buffer settings
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			// Attach textures to our bloomFBO
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, bloomColorBuffers[i], 0);
		}
		glGenRenderbuffers(1, &rboDepth);
		glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
		// Tell OpenGL to render to multiple colorbuffers through glDrawBuffers
		unsigned int attachmentsArray[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		glDrawBuffers(2, attachmentsArray);
		// Check framebuffer is complete
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			std::cout << "Framebuffer is not completely setup!" << std::endl;
		}
		else {
			std::cout << "Framebuffer is complete!" << std::endl;
		}
		// Reset back to normal
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void initializePingPongFBOs(int width, int height) {
		// Ping pong FBO for blurring
		glGenFramebuffers(2, pingPongFBO);
		glGenTextures(2, pingPongTextures);
		// Initialize both texture buffers
		for (unsigned int i = 0; i < 2; i++) {
			glBindFramebuffer(GL_FRAMEBUFFER, pingPongFBO[i]);
			glBindTexture(GL_TEXTURE_2D, pingPongTextures[i]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingPongTextures[i], 0);
			// Check framebuffer is complete
			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
				std::cout << "Framebuffer is not completely setup!" << std::endl;
			}
			else {
				std::cout << "Framebuffer is complete!" << std::endl;
			}
		}
	}

	void initializeShaderPrograms(const std::string& resource) {
		using ::std::cerr;
		using ::std::endl;

		// Initialize final shader program (merge bloom with normal photo)
		finalShader = new Program();
		finalShader->setVerbose(true);
		finalShader->setShaderNames(resource + "/blur_vert.glsl", resource + "/merge_frag.glsl");
		if (!finalShader->init()) {
			cerr << "One or more shaders failed to compile... exiting!" << endl;
			exit(EXIT_FAILURE);
		}
		finalShader->addUniform("scene");
		finalShader->addUniform("bloomBlur");
		finalShader->addAttribute("vertPos");
		finalShader->addAttribute("vertTex");

		// Initialize multisampling for final merge shader
		finalShader->bind();
		glUniform1i(finalShader->getUniform("scene"), 0);
		glUniform1i(finalShader->getUniform("bloomBlur"), 1);
		finalShader->unbind();

		// Initialize the bright filter program
		blurBloomShader = new Program();
		blurBloomShader->setVerbose(true);
		blurBloomShader->setShaderNames(resource + "/blur_vert.glsl", resource + "/blur_frag.glsl");
		if (!blurBloomShader->init()) {
			cerr << "One or more shaders failed to compile... exiting!" << endl;
			exit(EXIT_FAILURE);
		}
		blurBloomShader->addUniform("horizontal");
		blurBloomShader->addAttribute("vertPos");
		blurBloomShader->addAttribute("vertTex");

		// Initialize the scene program (blinn-phong shading on geometry)
		sceneShader = new Program();
		sceneShader->setVerbose(true);
		sceneShader->setShaderNames(resource + "/scene_vert.glsl", resource + "/scene_frag.glsl");
		if (!sceneShader->init()) {
			cerr << "One or more shaders failed to compile... exiting!" << endl;
			exit(EXIT_FAILURE);
		}
		sceneShader->addUniform("P");
		sceneShader->addUniform("V");
		sceneShader->addUniform("M");
		sceneShader->addUniform("isLightSource");
		sceneShader->addUniform("lights");
		sceneShader->addUniform("shininess");
		sceneShader->addUniform("shapeColor");
		sceneShader->addAttribute("vertPos");
		sceneShader->addAttribute("vertNor");
		sceneShader->addAttribute("vertTex");
	}

	void initializeGeometry(const std::string& resource)
	{
		initializeShapeFromFile(&globe, resource + "/globe.obj", &globeOffset);
		initializeShapeFromFile(&sphere, resource + "/sphere.obj", &sphereOffset);
		initializeShapeFromFile(&table, resource + "/table.obj", &tableOffset);
	}

	void initializeShapeFromFile(vector<Shape*>* inShape, const std::string& resource, vec3* offset) {
		using ::tinyobj::shape_t;
		using ::tinyobj::material_t;
		using ::tinyobj::LoadObj;

		using ::std::cerr;
		using ::std::endl;
		using ::std::string;

		vector<shape_t> shapes;
		vector<material_t> materials;
		string estr;

		bool rc = LoadObj(shapes, materials, estr, resource.c_str());

		if (!rc) {
			cerr << estr << endl;
		}
		else
		{
			for (shape_t s : shapes) {
				Shape* tempShape = new Shape();
				tempShape->createShape(s);
				tempShape->init();
				tempShape->measure();
				(*offset) += (tempShape->min + tempShape->max) / 2.0f;

				inShape->push_back(tempShape);
			}

			(*offset) /= inShape->size();
			std::cout << (*offset).x << " " << (*offset).y << " " << (*offset).z << " " << std::endl;
		}
	}

	float generateRandomFloat(float lowBound, float highBound) {
		return rand() % 100 / 100.0f * (highBound - lowBound) + lowBound;
	}

	void update(float dtime) {
		for (Particle* fly : fireflies) {
			// Update individual particle
			fly->update(dtime); 
			// Apply center is attractive if toggled
			if (isCenterPointAttractive) {
				// Get distance from center
				vec3 forceVectorTowardsCenter = centerPoint - fly->position;
				float distFromCenter = glm::length(forceVectorTowardsCenter) / 25.0f;

				fly->addForce(distFromCenter * generateRandomFloat(0.1f, 0.25f) * forceVectorTowardsCenter);
			}
			// Apply gravity if toggled
			if (isGravityOn) fly->addForce(vec3(0, -0.25f, 0));
			// Check repel magnets against fireflies
			for (Particle* mag : magnets) {
				// Apply repellant force away
				vec3 forceVector = fly->position - mag->position;
				if (glm::length(forceVector) < 1.0f) fly->addForce(forceVector);
			}
			// Apply small random force in any direction to simulate fly flight
			fly->addForce(vec3(generateRandomFloat(-0.15f, 0.15f), generateRandomFloat(-0.15f, 0.15f), generateRandomFloat(-0.15f, 0.15f)));
			// Check to make sure we don't surpass 500 (arbitrary limit, defined for shader because shaders don't like variable arrays?)
			if (fireflies.size() > NUMBER_OF_FIREFLIES - FIREFLIES_PER_CLICK) {
				fireflies.erase(fireflies.begin());
			}
		}
	}

	void render(float time) {
		// Get current frame buffer size
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		// Set window size
		glViewport(0, 0, width, height);
		
		// Bind and clear bloom framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, bloomFBO);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		// Draw objects to our bound FBO (bloomFBO) *** the scene shader has outputs to 2 color attachments ***
		drawObjects(width, height, time);
		
		// Reset framebuffer
		// Gaussian blur brightness
		gaussianBlurPingPongCode(width, height);

		// Bind and clear screen framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		finalShader->bind();
		// Bind normal scene photo texture
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, bloomColorBuffers[0]);
		// Bind bloom lights texture
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, pingPongTextures[!horizontal]);
		renderQuad();
		finalShader->unbind();
	}

	void gaussianBlurPingPongCode(int width, int height) {
		// Blurring iteration
		bool firstPass = true;
		int amount = 6;

		// Use shader on GPU for speed?
		blurBloomShader->bind();
		// Loop to blur
		for (unsigned int i = 0; i < amount; i++) {
			glBindFramebuffer(GL_FRAMEBUFFER, pingPongFBO[horizontal]);
			// Send horizontal uniform to GLSL
			glUniform1i(blurBloomShader->getUniform("horizontal"), horizontal);
			glActiveTexture(GL_TEXTURE0);
			// Bind texture to blur
			glBindTexture(GL_TEXTURE_2D, firstPass ? bloomColorBuffers[1] : pingPongTextures[!horizontal]);
			// Render billboard normally
			renderQuad();
			// Flip blur axis
			horizontal = !horizontal;
			if (firstPass) firstPass = false;
		}
		blurBloomShader->unbind();
	}

	unsigned int quadVAO = 0;
	unsigned int quadVBO;

	void renderQuad() {
		if (quadVAO == 0) {
			// Setup if not setup yet
			float quadVertices[] = {
				// positions        // texture Coords
				-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
				-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
				 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
				 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
			};
			
			// setup plane VAO
			glGenVertexArrays(1, &quadVAO);
			glGenBuffers(1, &quadVBO);
			glBindVertexArray(quadVAO);
			glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
		}
		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);
	}

	void drawObjects(int width, int height, float time) {
		using ::std::make_shared;
		using ::std::shared_ptr;

		// Create the matrix stacks
		shared_ptr<MatrixStack> P = make_shared<MatrixStack>();
		shared_ptr<MatrixStack> V = make_shared<MatrixStack>();
		shared_ptr<MatrixStack> M = make_shared<MatrixStack>();
		// Apply perspective projection for the fireflies
		P->pushMatrix();
		P->loadIdentity();
		P->perspective(45.0f, 1.0f * width / height, 0.01f, 100.0f);
		// View matrix
		V->pushMatrix();
		V->loadIdentity();
		V->lookAt(vec3(0, 0, 0), vec3(0, 0, -1), vec3(0, 1, 0));
		// Model matrix
		M->pushMatrix();
		M->loadIdentity();

		// Bind scene shader
		sceneShader->bind();
		// Generate light position arrays
		vec3 lightsArray[NUMBER_OF_FIREFLIES];
		for (int f = 0; f < fireflies.size(); f++) {
			lightsArray[f] = fireflies[f]->position;
		}
		// Draw fireflies
		for (Particle* fly : fireflies) {
			M->pushMatrix();
			M->translate(fly->position);
			M->scale(0.01f);

			glUniformMatrix4fv(sceneShader->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));
			glUniformMatrix4fv(sceneShader->getUniform("V"), 1, GL_FALSE, value_ptr(V->topMatrix()));
			glUniformMatrix4fv(sceneShader->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
			glUniform1i(sceneShader->getUniform("isLightSource"), 1);
			glUniform3fv(sceneShader->getUniform("lights"), NUMBER_OF_FIREFLIES, value_ptr(lightsArray[0]));
			glUniform3f(sceneShader->getUniform("shapeColor"), 0.85, 0.75, 0.60);
			glUniform1f(sceneShader->getUniform("shininess"), 15.0f);

			for (Shape* part : sphere) {
				part->draw(sceneShader);
			}

			M->popMatrix();
		}
		// Draw magnets
		for (Particle* ma : magnets) {
			M->pushMatrix();
			M->translate(ma->position);
			M->scale(0.1f);

			glUniformMatrix4fv(sceneShader->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));
			glUniformMatrix4fv(sceneShader->getUniform("V"), 1, GL_FALSE, value_ptr(V->topMatrix()));
			glUniformMatrix4fv(sceneShader->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
			glUniform1i(sceneShader->getUniform("isLightSource"), 0);
			glUniform3fv(sceneShader->getUniform("lights"), NUMBER_OF_FIREFLIES, value_ptr(lightsArray[0]));
			glUniform3f(sceneShader->getUniform("shapeColor"), 0.21, 0.21, 0.21);
			glUniform1f(sceneShader->getUniform("shininess"), 0.8f);

			for (Shape* s : sphere) {
				s->draw(sceneShader);
			}

			M->popMatrix();
		}

		// Translate scene back (instead of moving camera position, which we could do instead)
		M->translate(vec3(0, 0, -2));
		// Draw globe
		for (Shape* part : globe) {
			M->pushMatrix();
			M->scale(globeScale);
			M->translate(-globeOffset);

			glUniformMatrix4fv(sceneShader->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));
			glUniformMatrix4fv(sceneShader->getUniform("V"), 1, GL_FALSE, value_ptr(V->topMatrix()));
			glUniformMatrix4fv(sceneShader->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
			glUniform1i(sceneShader->getUniform("isLightSource"), 0);
			glUniform3fv(sceneShader->getUniform("lights"), NUMBER_OF_FIREFLIES, value_ptr(lightsArray[0]));
			glUniform3f(sceneShader->getUniform("shapeColor"), 0.33, 0.40, 0.50);
			glUniform1f(sceneShader->getUniform("shininess"), 1.2f);

			part->draw(sceneShader);

			M->popMatrix();
		}
		// Draw table
		for (Shape* t : table) {
			M->pushMatrix();
			M->translate(vec3(0, -0.68, 0));
			M->scale(tableScale);
			M->translate(-tableOffset);

			glUniformMatrix4fv(sceneShader->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));
			glUniformMatrix4fv(sceneShader->getUniform("V"), 1, GL_FALSE, value_ptr(V->topMatrix()));
			glUniformMatrix4fv(sceneShader->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
			glUniform1i(sceneShader->getUniform("isLightSource"), 0);
			glUniform3fv(sceneShader->getUniform("lights"), NUMBER_OF_FIREFLIES, value_ptr(lightsArray[0]));
			glUniform3f(sceneShader->getUniform("shapeColor"), 0.70, 0.40, 0.25);
			glUniform1f(sceneShader->getUniform("shininess"), 0.2f);

			t->draw(sceneShader);

			M->popMatrix();
		}

		// Unbind
		sceneShader->unbind();

		// Pop matrix stacks.
		M->popMatrix();
		V->popMatrix();
		P->popMatrix();
	}
};

int main(int argc, char** argv)
{
	float time = 0.0;
	float dtime = 0.1f;

	// Where the resources are loaded from
	std::string resources = (argc >= 2) ? argv[1] : "../resources";

	// Initialize our new application
	Application* application = new Application();

	// Your main will always include a similar set up to establish your window
	// and GL context, etc
	WindowManager* windowManager = new WindowManager();
	windowManager->init(1280, 720, "particle.io");
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

	// This is the code that will likely change program to program as you
	// may need to initialize or set up different data and state
	application->init(resources);

	// Loop until the user closes the window.
	while (!glfwWindowShouldClose(windowManager->getHandle()))
	{
		// Render scene.
		application->render(time);
		application->update(dtime);
		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();

		time += dtime;
	}
	// Quit program.
	windowManager->shutdown();
	exit(EXIT_SUCCESS);
}
