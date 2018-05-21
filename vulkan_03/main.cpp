#include "renderer.h"
#include "engine.h"

#include <exception>
#include <iostream>

int main()
{
	try
	{
		//renderer rendererObject;
		engine en;
	}
	catch(std::exception& e)
	{
		std::cout << "ERROR(exception): " << e.what() << std::endl;
		std::cin.ignore(); std::cin.get(); // dont close console imediately
		return -1;
	}
	catch (...)
	{
		std::cout << "ERROR: unknown exception caught" << std::endl;
		std::cin.ignore(); std::cin.get(); // dont close console imediately
		return -1;
	}

	return 0;
}