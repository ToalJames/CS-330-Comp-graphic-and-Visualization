///////////////////////////////////////////////////////////////////////////////
// viewmanager.cpp
// ============
// manage the viewing of 3D objects within the viewport
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "ViewManager.h"

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>    

// Namespace for managing global/static state for callbacks
namespace
{
	// Variables for window width and height
	const int WINDOW_WIDTH = 1000;
	const int WINDOW_HEIGHT = 800;
	const char* g_ViewName = "view";
	const char* g_ProjectionName = "projection";

	// Internal pointer used for static callback access
	Camera* g_pCamera = nullptr;

	// Mouse processing state
	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	bool gFirstMouse = true;

	// Timing state
	float gDeltaTime = 0.0f;
	float gLastFrame = 0.0f;

	// Projection toggle state
	bool bOrthographicProjection = false;
}

/***********************************************************
 * ViewManager()
 * * Constructor initializes the camera and synchronizes
 * the pointers for the SceneManager to access.
 ***********************************************************/
ViewManager::ViewManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_pWindow = NULL;

	// Initialize the camera object
	g_pCamera = new Camera();

	// SYNC: Link the class member m_pCamera to the global g_pCamera
	// This ensures GetCamera() returns a valid address for lighting.
	m_pCamera = g_pCamera;

	// Default camera view parameters
	g_pCamera->Position = glm::vec3(0.0f, 5.0f, 12.0f);
	g_pCamera->Front = glm::vec3(0.0f, -0.5f, -2.0f);
	g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
	g_pCamera->Zoom = 45.0f;

	// Set initial movement speed
	g_pCamera->MovementSpeed = 5.0f;
}

/***********************************************************
 * ~ViewManager()
 ***********************************************************/
ViewManager::~ViewManager()
{
	m_pShaderManager = NULL;
	m_pWindow = NULL;

	if (NULL != g_pCamera)
	{
		delete g_pCamera;
		g_pCamera = NULL;
		m_pCamera = NULL;
	}
}

/***********************************************************
 * CreateDisplayWindow()
 ***********************************************************/
GLFWwindow* ViewManager::CreateDisplayWindow(const char* windowTitle)
{
	GLFWwindow* window = nullptr;

	window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, windowTitle, NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return NULL;
	}
	glfwMakeContextCurrent(window);

	// Registration for input callbacks
	glfwSetCursorPosCallback(window, &ViewManager::Mouse_Position_Callback);
	glfwSetScrollCallback(window, &ViewManager::Mouse_Scroll_Callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Support for transparent/blended rendering
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_pWindow = window;
	return(window);
}

/***********************************************************
 * Mouse_Position_Callback()
 ***********************************************************/
void ViewManager::Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos)
{
	if (gFirstMouse)
	{
		gLastX = xMousePos;
		gLastY = yMousePos;
		gFirstMouse = false;
	}

	float xOffset = xMousePos - gLastX;
	float yOffset = gLastY - yMousePos;

	gLastX = xMousePos;
	gLastY = yMousePos;

	if (g_pCamera != nullptr)
		g_pCamera->ProcessMouseMovement(xOffset, yOffset);
}

/***********************************************************
 * Mouse_Scroll_Callback()
 ***********************************************************/
void ViewManager::Mouse_Scroll_Callback(GLFWwindow* window, double xOffset, double yOffset)
{
	if (g_pCamera != nullptr)
	{
		g_pCamera->MovementSpeed += (float)yOffset * 0.5f;
		if (g_pCamera->MovementSpeed < 0.1f) g_pCamera->MovementSpeed = 0.1f;
		if (g_pCamera->MovementSpeed > 20.0f) g_pCamera->MovementSpeed = 20.0f;
	}
}
/***********************************************************
 * RenderHUD()
 * Renders a 2D overlay listing all camera control commands.
 * Uses a fixed orthographic projection independent of the
 * scene camera so the HUD is always screen-aligned.
 ***********************************************************/
void ViewManager::RenderHUD()
{
	// HUD rendering requires a text library such as FreeType or
	// a bitmap font. Since the project uses raw OpenGL, the most
	// practical option is to print controls to the window title bar
	// which is always visible and requires no additional dependencies.

	std::string mode = bOrthographicProjection ? "ORTHO" : "PERSPECTIVE";
	std::string speed = std::to_string((int)g_pCamera->MovementSpeed);

	std::string title =
		"7-1 Final  |  Mode: " + mode +
		"  |  Speed: " + speed +
		"  |  W/S=Fwd/Back  A/D=Left/Right  Q/E=Up/Down" +
		"  |  Mouse=Look  Scroll=Speed  O=Ortho  P=Persp";

	glfwSetWindowTitle(m_pWindow, title.c_str());
}
/***********************************************************
 * ProcessKeyboardEvents()
 ***********************************************************/
void ViewManager::ProcessKeyboardEvents()
{	
	static bool pKeyWasPressed = false;
	static bool oKeyWasPressed = false;

	// Replace the existing P/O key blocks with:
	bool pKeyNow = glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_PRESS;
	if (pKeyNow && !pKeyWasPressed)
		bOrthographicProjection = false; // switch to perspective on tap
	pKeyWasPressed = pKeyNow;

	bool oKeyNow = glfwGetKey(m_pWindow, GLFW_KEY_O) == GLFW_PRESS;
	if (oKeyNow && !oKeyWasPressed)
		bOrthographicProjection = true;  // switch to orthographic on tap
	oKeyWasPressed = oKeyNow;
	if (glfwGetKey(m_pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(m_pWindow, true);
	}

	if (g_pCamera != nullptr)
	{
		if (glfwGetKey(m_pWindow, GLFW_KEY_W) == GLFW_PRESS)
			g_pCamera->ProcessKeyboard(FORWARD, gDeltaTime);
		if (glfwGetKey(m_pWindow, GLFW_KEY_S) == GLFW_PRESS)
			g_pCamera->ProcessKeyboard(BACKWARD, gDeltaTime);
		if (glfwGetKey(m_pWindow, GLFW_KEY_A) == GLFW_PRESS)
			g_pCamera->ProcessKeyboard(LEFT, gDeltaTime);
		if (glfwGetKey(m_pWindow, GLFW_KEY_D) == GLFW_PRESS)
			g_pCamera->ProcessKeyboard(RIGHT, gDeltaTime);
		if (glfwGetKey(m_pWindow, GLFW_KEY_Q) == GLFW_PRESS)
			g_pCamera->ProcessKeyboard(UP, gDeltaTime);
		if (glfwGetKey(m_pWindow, GLFW_KEY_E) == GLFW_PRESS)
			g_pCamera->ProcessKeyboard(DOWN, gDeltaTime);

		if (glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_PRESS)
			bOrthographicProjection = false;
		if (glfwGetKey(m_pWindow, GLFW_KEY_O) == GLFW_PRESS)
			bOrthographicProjection = true;
	}
}

/***********************************************************
 * PrepareSceneView()
 ***********************************************************/
void ViewManager::PrepareSceneView()
{
	glm::mat4 view;
	glm::mat4 projection;

	float currentFrame = glfwGetTime();
	gDeltaTime = currentFrame - gLastFrame;
	gLastFrame = currentFrame;
	RenderHUD(); // update title bar with current mode and controls
	ProcessKeyboardEvents();

	if (g_pCamera != nullptr)
	{
		view = g_pCamera->GetViewMatrix();

		if (!bOrthographicProjection)
		{
			projection = glm::perspective(glm::radians(g_pCamera->Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
		}
		else
		{
			float scale = 5.0f;
			float aspectRatio = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;
			projection = glm::ortho(-aspectRatio * scale, aspectRatio * scale, -scale, scale, 0.1f, 100.0f);
		}

		if (NULL != m_pShaderManager)
		{
			m_pShaderManager->setMat4Value(g_ViewName, view);
			m_pShaderManager->setMat4Value(g_ProjectionName, projection);
			// Update the viewPosition specifically for Phong specular logic
			m_pShaderManager->setVec3Value("viewPosition", g_pCamera->Position);
		}
	}
}