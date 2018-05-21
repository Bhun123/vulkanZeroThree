#pragma once

#include <GLFW/glfw3.h>
#include "camera.h"

void glfw_key_callback(GLFWwindow * window, int key, int scancode, int action, int mode);
void glfw_mouse_callback(GLFWwindow* window, double xpos, double ypos);

/* GLFW window instance (and cam) needs to be created before this. */
class ioHandler
{
public:
	ioHandler(GLFWwindow * mWindow, camera & mCamera);
	~ioHandler();

	//temp
	void initialize(GLFWwindow * window);
	
	void keyCallback(uint32_t key, uint32_t scancode, uint32_t action, uint32_t mode);
	void mouseCallback(double xpos, double ypos);

	//prec stym
	bool keys[1024]; // uz neviem aaaaaaaaaaaaaaaaaaaaaaaaaaaa
	double mouseLastX, mouseLastY;
	void callEveryFrameNeviemBlbost();

	bool mWindowHasFocus = true;
	GLFWwindow * mWindow;
	camera & mCamera;
};

