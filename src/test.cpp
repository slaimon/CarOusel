#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>  
#include <glm/ext.hpp>

#include "../common/debugging.h"
#include "../common/renderable.h"
#include "../common/shaders.h"
#include "../common/simple_shapes.h"

#include "camera_controls.h"

int width = 800;
int height = 800;
float radius = 5.f;
//float camX, camZ;

CameraControls camera(glm::vec3(radius, 0.0f, 0.0f), glm::vec3(0.0f));

float deltaTime = 0.0f;
float lastFrame = 0.0f;

void updateDelta() {
   float currentFrame = glfwGetTime();
   deltaTime = currentFrame - lastFrame;
   lastFrame = currentFrame;
}

void processInput(GLFWwindow* window) {
   if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
      glfwSetWindowShouldClose(window, true);

   if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
      camera.step(CAMERA_AHEAD, deltaTime);
   if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
      camera.step(CAMERA_BACK, deltaTime);
   if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
      camera.step(CAMERA_LEFT, deltaTime);
   if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
      camera.step(CAMERA_RIGHT, deltaTime);
   if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
      camera.step(CAMERA_UP, deltaTime);
   if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
      camera.step(CAMERA_DOWN, deltaTime);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
   camera.mouseLook(xpos/width, ypos/height);
}

int main(void) {
	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(height, width, "test", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}
	
	glfwMakeContextCurrent(window);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(window, mouse_callback);
	glewInit();
	printout_opengl_glsl_info();

	/* load the shaders */
	shader basic_shader;
	basic_shader.create_program("shaders/test.basic.vert", "shaders/test.basic.frag");
	
	/* create a cube centered at the origin with side 2*/
	renderable r_cube = shape_maker::cube(0.5f, 0.3f, 0.0f);

	check_gl_errors(__LINE__, __FILE__);
	
	glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)width/height, 1.0f, 25.0f);
	
	glUseProgram(basic_shader.program);
	glUniformMatrix4fv(basic_shader["uProj"], 1, GL_FALSE, &proj[0][0]);
	glUniformMatrix4fv(basic_shader["uView"], 1, GL_FALSE, &camera.matrix()[0][0]);
	glUniformMatrix4fv(basic_shader["uModel"], 1, GL_FALSE, &glm::mat4(1.f)[0][0]);
	glUniform3f(basic_shader["uColor"], 1.0,0.0,0.0);
	
	glEnable(GL_DEPTH_TEST);
	while(!glfwWindowShouldClose(window)) {
	   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	   glClearColor(1,1,1,1);
	   check_gl_errors(__LINE__,__FILE__);
	   
	   /*
	   // continuous rotation around the origin
	   camX = cos(glfwGetTime()) * radius;
	   camZ = sin(glfwGetTime()) * radius;
	   camera.set(glm::vec3(camX, 0.0f, camZ), glm::vec3(0.0f));
	   */
	   updateDelta();
	   processInput(window);
   	glUniformMatrix4fv(basic_shader["uView"], 1, GL_FALSE, &camera.matrix()[0][0]);
   	
	   r_cube.bind();
	   glDrawElements(r_cube().mode, r_cube().count, r_cube().itype, 0);
	   
		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();
	}
	
	glUseProgram(0);
	glfwTerminate();
	
	return 0;
}
