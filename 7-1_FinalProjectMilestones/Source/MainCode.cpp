#include <iostream>          // error handling and output
#include <cstdlib>           // EXIT_FAILURE

#include <GL/glew.h>         // GLEW library
#include "GLFW/glfw3.h"      // GLFW library

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "SceneManager.h"
#include "ViewManager.h"
#include "ShapeMeshes.h"
#include "ShaderManager.h"

// Namespace for declaring global variables
namespace
{
	// Macro for window title
	const char* const WINDOW_TITLE = "7-1 FinalProject and Milestones";

	// Main GLFW window
	GLFWwindow* g_Window = nullptr;

	// scene manager object for managing the 3D scene prepare and render
	SceneManager* g_SceneManager = nullptr;
	// shader manager object for dynamic interaction with the shader code
	ShaderManager* g_ShaderManager = nullptr;
	// view manager object for managing the 3D view setup and projection to 2D
	ViewManager* g_ViewManager = nullptr;
}

// Function declarations
bool InitializeGLFW();
bool InitializeGLEW();

/***********************************************************
 * main(int, char*)
 *
 * Entry point for the 3D kitchen scene application.
 ***********************************************************/
int main(int argc, char* argv[])
{
	// if GLFW fails initialization, then terminate the application
	if (InitializeGLFW() == false)
	{
		return(EXIT_FAILURE);
	}

	// try to create a new shader manager object
	g_ShaderManager = new ShaderManager();

	// try to create a new view manager object
	g_ViewManager = new ViewManager(g_ShaderManager);

	// try to create the main display window
	g_Window = g_ViewManager->CreateDisplayWindow(WINDOW_TITLE);

	// if GLEW fails initialization, then terminate the application
	if (InitializeGLEW() == false)
	{
		return(EXIT_FAILURE);
	}

	// load the shader code from the external GLSL files
	g_ShaderManager->LoadShaders(
		"../../Utilities/shaders/vertexShader.glsl",
		"../../Utilities/shaders/fragmentShader.glsl");
	g_ShaderManager->use();

	// Create the scene manager object 
	g_SceneManager = new SceneManager(g_ShaderManager);

	// --- CAMERA LINKING (CRITICAL FOR LIGHTING) ---
	// Passes the camera pointer from ViewManager to SceneManager.
	// This enables the calculation of 'viewPosition' required for specular highlights.
	if (g_ViewManager != nullptr && g_SceneManager != nullptr)
	{
		g_SceneManager->SetCamera(g_ViewManager->GetCamera());
	}

	/**
	 * Configure Global OpenGL State
	 * * Enabling Depth Testing ensures proper occlusion and object layering.
	 * Alpha Blending is enabled to support transparency for window glass and materials
	 * utilizing the alpha channel.
	 */
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Prepare the 3D scene (loads meshes and textures)
	g_SceneManager->PrepareScene();

	// Main render loop
	while (!glfwWindowShouldClose(g_Window))
	{
		// Clear the frame and z buffers
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Convert from 3D object space to 2D view
		g_ViewManager->PrepareSceneView();

		// Refresh and render the 3D scene objects
		g_SceneManager->RenderScene();

		// Flips the back buffer with the front buffer every frame.
		glfwSwapBuffers(g_Window);

		// Query the latest GLFW events
		glfwPollEvents();
	}

	// Clear the allocated manager objects from memory
	if (nullptr != g_SceneManager)
	{
		delete g_SceneManager;
		g_SceneManager = nullptr;
	}
	if (nullptr != g_ViewManager)
	{
		delete g_ViewManager;
		g_ViewManager = nullptr;
	}
	if (nullptr != g_ShaderManager)
	{
		delete g_ShaderManager;
		g_ShaderManager = nullptr;
	}

	exit(EXIT_SUCCESS);
}

/***********************************************************
 *	InitializeGLFW()
 ***********************************************************/
bool InitializeGLFW()
{
	glfwInit();

#ifdef __APPLE__
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

	return(true);
}

/***********************************************************
 *	InitializeGLEW()
 ***********************************************************/
bool InitializeGLEW()
{
	GLenum GLEWInitResult = GLEW_OK;
	GLEWInitResult = glewInit();
	if (GLEW_OK != GLEWInitResult)
	{
		std::cerr << glewGetErrorString(GLEWInitResult) << std::endl;
		return false;
	}

	std::cout << "INFO: OpenGL Successfully Initialized\n";
	std::cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << "\n" << std::endl;

	return(true);
}