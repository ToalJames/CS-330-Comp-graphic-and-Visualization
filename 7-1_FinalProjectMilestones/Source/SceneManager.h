///////////////////////////////////////////////////////////////////////////////
// shadermanager.h
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ShaderManager.h"
#include "ShapeMeshes.h"
#include "camera.h"

#include <string>
#include <vector>

// Structure for material properties matching the Phong lighting model
struct OBJECT_MATERIAL
{
	glm::vec3 ambientColor;
	float ambientStrength;
	glm::vec3 diffuseColor;
	glm::vec3 specularColor;
	float shininess;
	std::string tag;
};

class SceneManager
{
public:
	SceneManager(ShaderManager* pShaderManager);
	~SceneManager();

	// --- CAMERA LINKING ---
	// Receives the camera pointer from main.cpp to enable lighting reflections
	void SetCamera(Camera* pCamera) { m_pCamera = pCamera; }

	// prepares the 3D scene objects and textures
	void PrepareScene();

	// renders the 3D scene
	void RenderScene();

private:
	// loads object material presets into the collection
	void DefineObjectMaterials();

	// selects a material preset by tag and sends uniforms to the shader
	void SelectMaterial(std::string tag);

	// sets transformation matrices for the currently selected object
	void SetTransformations(glm::vec3 scaleXYZ, float XrotationDegrees, float YrotationDegrees, float ZrotationDegrees, glm::vec3 positionXYZ);

	// sets the basic color for the object if textures are not used
	void SetShaderColor(float r, float g, float b, float a);

	// sets the texture and UV scale for the object
	void SetShaderTexture(GLuint textureId, glm::vec2 uvScale);

	// creates a texture from an image file
	bool CreateTexture(const char* filename, GLuint& textureId);

	// configures the scene lighting (ambient, point light, and view position)
	void SetupSceneLights();

	// Rendering sub-functions for kitchen components
	void RenderRoom();
	void RenderWindow();
	void RenderSink();

private:
	ShaderManager* m_pShaderManager; // Pointer to the shader manager
	ShapeMeshes* m_basicMeshes;     // Pointer to the mesh generator
	Camera* m_pCamera;              // Pointer to the scene camera

	// Vector to store defined materials
	std::vector<OBJECT_MATERIAL> m_objectMaterials;
};