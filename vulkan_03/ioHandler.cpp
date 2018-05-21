#include "ioHandler.h"

void glfw_key_callback(GLFWwindow * window, int key, int scancode, int action, int mode)
{
	static_cast<ioHandler*>(glfwGetWindowUserPointer(window))->keyCallback(key, scancode, action, mode);
}

void glfw_mouse_callback(GLFWwindow * window, double xpos, double ypos)
{
	static_cast<ioHandler*>(glfwGetWindowUserPointer(window))->mouseCallback(xpos, ypos);
}

ioHandler::ioHandler(GLFWwindow * window, camera & camera)
	: mWindow(window), mCamera(camera)
{
}

ioHandler::~ioHandler()
{
}

void ioHandler::initialize(GLFWwindow * window)
{
	mWindow = window;
	glfwSetKeyCallback(mWindow, glfw_key_callback);
	glfwSetCursorPosCallback(mWindow, glfw_mouse_callback);

	glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetWindowUserPointer(mWindow, this);
}

void ioHandler::callEveryFrameNeviemBlbost()
{
	if (keys[GLFW_KEY_W]) mCamera.move(cameraMovement::forward);
	if (keys[GLFW_KEY_S]) mCamera.move(cameraMovement::backward);
	if (keys[GLFW_KEY_A]) mCamera.move(cameraMovement::left);
	if (keys[GLFW_KEY_D]) mCamera.move(cameraMovement::right);
}

void ioHandler::keyCallback(uint32_t key, uint32_t scancode, uint32_t action, uint32_t mode)
{
	switch (key)
	{
	case GLFW_KEY_BACKSPACE:
	{
		if (action == GLFW_PRESS)
			if (mWindowHasFocus)
			{
				glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				mWindowHasFocus = false;
			}
			else
			{
				glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				mWindowHasFocus = true;
			}
	}
	break;

	case GLFW_KEY_ESCAPE:
		glfwSetWindowShouldClose(mWindow, 1);
		break;

	default:
		break;
	}

	mCamera.keyAction(key, action);
}

void ioHandler::mouseCallback(double xpos, double ypos)
{
	mCamera.mouseAction(xpos, ypos);
}
