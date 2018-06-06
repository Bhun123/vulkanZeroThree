#include "engine.h"

#include <Windows.h>

engine::engine()
	:mIoHandler(mWindow, mCamera)
{
	createWindow();
	//temp
	mRenderer.mWindow = mWindow;

	models.push_back(model());
	models[0].loadModel("models/chalet.obj");

	mRenderer.initialize();
	mainLoop();

}


engine::~engine()
{
}

void engine::mainLoop()
{
	while(!glfwWindowShouldClose(mWindow))
	{
		mRenderer.updateMatrix(mCamera.getViewMatrix());
		mRenderer.frame();
		
		//temp
		static auto startTime = std::chrono::high_resolution_clock::now();
		auto currentTime = std::chrono::high_resolution_clock::now();
		double_t time = std::chrono::duration<double_t>(currentTime - startTime).count(); 
		
		double_t dTime = time - lastT; lastT = time; 
		avg += dTime; cnt++;
		if (avg >= 1.0f) {
			std::cout << "fps:" << 1 / (avg / cnt) << std::endl; cnt = 0; avg = 0.0f;
		}
		Sleep(19);

		glfwPollEvents(); 
		mCamera.move(time); 
	}
}

void engine::createWindow()
{
	if (glfwInit() == GLFW_FALSE) throw std::runtime_error("failed to create window!");

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	mWindow = glfwCreateWindow(mRenderer.WIN_WIDTH, mRenderer.WIN_HEIGHT, "vulkanEngine", nullptr, nullptr);
	mIoHandler.initialize(mWindow);
}


