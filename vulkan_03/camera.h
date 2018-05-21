#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include "GLFW\glfw3.h"

#include <iostream>

enum cameraMovement
{
	forward,
	backward,
	left,
	right
};

//General camera needlessly complicates code
/*specialized camera class for glfw*/
class camera
{
public:
	camera();
	virtual ~camera();

	float_t speed, mouseSensitivity;
	float_t pitch, yaw, roll;
	double_t lastFrameTime = 0.0f;
	glm::vec3 cameraFront, cameraUp, cameraPos;
	glm::vec2 mouseOffset = { 0.0f, 0.0f }, mouseLast = { 0.0f, 0.0f };
	bool keys[1024] = {};

	bool cameraNotMoveNextFrame = true;

	//write with quaternion(not now)
	glm::fquat hit = glm::angleAxis(2.0f, glm::vec3(2.0f, 2.0f, 2.0f));

	void move(double_t time);

	void keyAction(uint32_t key, uint32_t action);
	void mouseAction(double_t xpos, double_t ypos);

	//temp 
	glm::mat4 getViewMatrix();
};

