#pragma once

#include "renderer.h"
#include "camera.h"
#include "ioHandler.h"

#include <iostream>

class engine
{
public:
	engine();
	virtual ~engine();

	void mainLoop();

	void createWindow();

	renderer mRenderer;
	camera mCamera;
	GLFWwindow * mWindow;
	ioHandler mIoHandler;

	double_t lastT = 0, avg = 0; uint32_t cnt = 0;
};


