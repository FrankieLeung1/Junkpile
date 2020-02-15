#pragma once

#include "../Resources/ResourceManager.h"
class glfwFramework : public SingletonResource<glfwFramework>
{
public:
	glfwFramework();
	~glfwFramework();

protected:
	//GLFWwindow* m_window;
};