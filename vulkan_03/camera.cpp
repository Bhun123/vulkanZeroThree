#include "camera.h"

camera::camera()
{
	speed = 5.0f;
	mouseSensitivity = 0.1f;
	cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

	cameraFront = glm::vec3(0.0f, 0.0f, 1.0f);
	cameraPos = glm::vec3(0.0f, 0.0f, -16.0f);

	yaw = 90.0f;
	pitch = 0.0f;
}


camera::~camera()
{
}


void camera::move(double_t engineTime)
{
	double_t dTime = engineTime - lastFrameTime;
	
	float_t cameraSpeed = speed * dTime;
	lastFrameTime = engineTime;

	if (keys[GLFW_KEY_W])
		cameraPos += cameraSpeed * cameraFront;
	if (keys[GLFW_KEY_S])
		cameraPos -= cameraSpeed * cameraFront;
	if (keys[GLFW_KEY_A])
		cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	if (keys[GLFW_KEY_D])
		cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

	// calculate camera rotation
	mouseOffset *= mouseSensitivity;

	yaw = glm::mod(yaw + mouseOffset.x, 360.0f);
	pitch += mouseOffset.y;

	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	glm::vec3 front;
	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

	cameraFront = glm::normalize(front);
	//std::cout << mouseOffset.x << " " << mouseOffset.y << std::endl;
	mouseOffset = { 0.0f, 0.0f }; 
}

void camera::keyAction(uint32_t key, uint32_t action)
{
	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
			keys[key] = true;
		else if (action == GLFW_RELEASE)
			keys[key] = false;
	}
}

void camera::mouseAction(double_t xpos, double_t ypos)
{
	// calculate mouse offsets. will be reset at next frame.
	mouseOffset.x += xpos - mouseLast.x;
	mouseOffset.y += mouseLast.y - ypos;
	mouseLast.x = xpos;
	mouseLast.y = ypos;

	if (cameraNotMoveNextFrame)
	{
		mouseOffset = { 0.0f, 0.0f };
		cameraNotMoveNextFrame = false;
	}

	//glm::quat tempQuat = glm::quat(glm::vec3(pitch, yaw, roll));

	//std::cout << pitch << " " << yaw << std::endl;
}

glm::mat4 camera::getViewMatrix()
{
	return glm::lookAt(cameraPos, cameraFront + cameraPos, cameraUp);
}
