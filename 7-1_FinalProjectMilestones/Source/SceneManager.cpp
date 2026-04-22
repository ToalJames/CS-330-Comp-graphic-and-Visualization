///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// Global variable definitions for shader uniform naming consistency
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
	const char* g_TextureScaleName = "uvScale";

	// Lighting and Material uniform names matching the Phong fragment shader
	const char* g_LightColorName = "lightColor";
	const char* g_LightPosName = "lightPos";
	const char* g_AmbientColorName = "ambientColor";
	const char* g_ViewPosName = "viewPosition";

	// Texture handles for OpenGL material mapping
	GLuint gTexturePlaster;
	GLuint gTextureMarble;
	GLuint gTextureFloor;
	GLuint gTextureWood;
}

/**
 * Constructor: Initializes core managers and populates the material library.
 */
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
	m_pCamera = nullptr;

	// Initialize the material presets for the scene
	DefineObjectMaterials();
}

/**
 * Destructor: Cleans up memory for shape meshes.
 */
SceneManager::~SceneManager()
{
	m_pShaderManager = nullptr;
	delete m_basicMeshes;
	m_basicMeshes = nullptr;
}

/**
 * Material Library: Defines the physical properties of different surfaces.
 * These values are calibrated to prevent specular blowout and excessive ambient glow.
 */
void SceneManager::DefineObjectMaterials()
{
	// --- FLOOR MATERIAL ---
	OBJECT_MATERIAL floorMat;
	floorMat.ambientColor = glm::vec3(0.45f, 0.38f, 0.28f);
	floorMat.ambientStrength = 0.25f;                          
	floorMat.diffuseColor = glm::vec3(0.55f, 0.48f, 0.38f); 
	floorMat.specularColor = glm::vec3(0.1f, 0.1f, 0.08f);
	floorMat.shininess = 16.0f;
	floorMat.tag = "floor";
	m_objectMaterials.push_back(floorMat);

	// --- LIGHT-ABSORBENT MATERIAL ---
	OBJECT_MATERIAL darkWall;
	darkWall.ambientColor = glm::vec3(0.30f, 0.28f, 0.24f);
	darkWall.ambientStrength = 0.20f;
	darkWall.diffuseColor = glm::vec3(0.28f, 0.26f, 0.22f); 
	darkWall.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	darkWall.shininess = 1.0f;
	darkWall.tag = "darkWall";
	m_objectMaterials.push_back(darkWall);

	// --- POLISHED MATERIAL (Marble, Metal) ---
	OBJECT_MATERIAL shiny;
	shiny.ambientColor = glm::vec3(0.15f, 0.15f, 0.15f);
	shiny.ambientStrength = 0.1f;
	shiny.diffuseColor = glm::vec3(0.65f, 0.65f, 0.65f);
	shiny.specularColor = glm::vec3(0.4f, 0.4f, 0.35f);
	shiny.shininess = 64.0f;  // tighter highlight
	shiny.tag = "shiny";
	m_objectMaterials.push_back(shiny);

	// --- MATTE PLASTER MATERIAL (Walls) ---
	OBJECT_MATERIAL clay;
	clay.ambientColor = glm::vec3(0.25f, 0.25f, 0.25f);
	clay.ambientStrength = 0.1f;
	clay.diffuseColor = glm::vec3(0.35f, 0.35f, 0.35f);
	clay.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	clay.shininess = 1.0f;
	clay.tag = "clay";
	m_objectMaterials.push_back(clay);

	// --- STANDARD MATTE MATERIAL (Wood) ---
	OBJECT_MATERIAL standard;
	standard.ambientColor = glm::vec3(0.35f, 0.28f, 0.18f); 
	standard.ambientStrength = 0.3f;   
	standard.diffuseColor = glm::vec3(0.55f, 0.45f, 0.30f);
	standard.specularColor = glm::vec3(0.05f, 0.05f, 0.03f);
	standard.shininess = 4.0f;
	standard.tag = "standard";
	m_objectMaterials.push_back(standard);
}

/**
 * Passes selected material uniforms to the GPU for the current draw call.
 */
void SceneManager::SelectMaterial(std::string tag)
{
	for (const auto& material : m_objectMaterials)
	{
		if (material.tag == tag)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
			return;
		}
	}
}

/**
 * Loads image files and generates OpenGL texture IDs.
 */
bool SceneManager::CreateTexture(const char* filename, GLuint& textureId)
{
	int width, height, channels;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);

	if (image)
	{
		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (channels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		else if (channels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

		glGenerateMipmap(GL_TEXTURE_2D);
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0);
		return true;
	}
	return false;
}

/**
 * Binds a texture and sets the UV scale uniform.
 */
void SceneManager::SetShaderTexture(GLuint textureId, glm::vec2 uvScale)
{
	if (nullptr != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);
		m_pShaderManager->setVec2Value(g_TextureScaleName, uvScale);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureId);
	}
}

/**
 * Sets object color and disables textures for solid-colored shapes.
 */
void SceneManager::SetShaderColor(float r, float g, float b, float a)
{
	if (nullptr != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, glm::vec4(r, g, b, a));
	}
}

/**
 * Calculates and sends the Model-View matrix to the shader.
 */
void SceneManager::SetTransformations(glm::vec3 scaleXYZ, float XrotationDegrees, float YrotationDegrees, float ZrotationDegrees, glm::vec3 positionXYZ)
{
	glm::mat4 modelView;
	glm::mat4 scale = glm::scale(scaleXYZ);
	glm::mat4 rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	glm::mat4 rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4 translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (nullptr != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/**
 * Configures the global illumination and primary point light for the scene.
 * Technical Note: Intensity values are normalized to prevent color clamping
 * and preserve high-frequency texture details.
 */
void SceneManager::SetupSceneLights()
{
	m_pShaderManager->setIntValue(g_UseLightingName, true);
	m_pShaderManager->setVec3Value(g_ViewPosName, m_pCamera->Position);

	// Light 0 — window sunlight entering from the back-right aperture.
	m_pShaderManager->setVec3Value("lightSources[0].position", glm::vec3(4.0f, 7.0f, -7.0f));
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", glm::vec3(0.0f, 0.0f, 0.0f));
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", glm::vec3(1.0f, 0.95f, 0.82f));
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", glm::vec3(0.3f, 0.3f, 0.25f));
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.3f);

	// Light 1 — warm incandescent bulb centred above the sink cabinet.
	m_pShaderManager->setVec3Value("lightSources[1].position", glm::vec3(-1.0f, 3.0f, 0.0f));
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", glm::vec3(0.15f, 0.10f, 0.05f));
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", glm::vec3(0.6f, 0.45f, 0.25f));
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", glm::vec3(0.1f, 0.08f, 0.04f));
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 8.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.1f);

	// Light 2 — low-intensity fill simulating diffuse inter-reflection
	// from the plaster walls.  No specular contribution; purpose is
	// solely to lift shadow regions above pure black.
	m_pShaderManager->setVec3Value("lightSources[2].position", glm::vec3(5.0f, 6.0f, 2.0f));
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", glm::vec3(0.12f, 0.11f, 0.09f));
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", glm::vec3(0.25f, 0.23f, 0.20f));
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", glm::vec3(0.0f, 0.0f, 0.0f));
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 2.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.0f);

	// Light 3 — disabled; this exists just toget rid of memory of previous light3
	m_pShaderManager->setVec3Value("lightSources[3].ambientColor", glm::vec3(0.0f, 0.0f, 0.0f));
	m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", glm::vec3(0.0f, 0.0f, 0.0f));
	m_pShaderManager->setVec3Value("lightSources[3].specularColor", glm::vec3(0.0f, 0.0f, 0.0f));
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 1.0f);
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 0.0f);
}

/**
 * Loads meshes and textures into memory.
 */
void SceneManager::PrepareScene()
{
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadConeMesh();

	CreateTexture("plaster_material.jpg", gTexturePlaster);
	CreateTexture("marble_material.jpg", gTextureMarble);
	CreateTexture("floor_material.jpg", gTextureFloor);
	CreateTexture("wood_texture.jpg", gTextureWood);
}

/**
 * Main render loop entry point.
 */
void SceneManager::RenderScene()
{
	SetupSceneLights(); // Prime the lighting uniforms before rendering objects
	RenderRoom();
	RenderSink();
	RenderWindow();
}

/**
 * Renders the structural environment including floor and walls.
 * The room volume is scaled to 20x12x20 to satisfy lighting rubric requirements.
 */
void SceneManager::RenderRoom()
{
	// Floor slab — UV tiled 2x to preserve texel density at 20-unit scale.
	SelectMaterial("floor");
	SetShaderTexture(gTextureFloor, glm::vec2(2.0f, 2.0f));
	SetTransformations(glm::vec3(20.0f, 0.2f, 20.0f), 0, 0, 0, glm::vec3(0.0f, -0.1f, 0.0f));
	m_basicMeshes->DrawBoxMesh();

	// Walls — shared plaster texture at 1:1 UV scale.
	SelectMaterial("darkWall");
	SetShaderTexture(gTexturePlaster, glm::vec2(1.0f, 1.0f));

	SetTransformations(glm::vec3(20.0f, 15.0f, 0.5f), 0, 0, 0, glm::vec3(0.0f, 7.5f, -10.0f));
	m_basicMeshes->DrawBoxMesh();

	SetTransformations(glm::vec3(0.5f, 15.0f, 20.0f), 0, 0, 0, glm::vec3(-10.0f, 7.5f, 0.0f));
	m_basicMeshes->DrawBoxMesh();

	// Pan 1 — body
	SetShaderColor(0.15f, 0.12f, 0.10f, 1.0f);
	SelectMaterial("shiny");
	SetTransformations(glm::vec3(0.7f, 0.1f, 0.7f), 0, 0, 90.0f, glm::vec3(-9.6f, 8.5f, -4.5f));
	m_basicMeshes->DrawCylinderMesh();

	// Pan 1 — handle projecting upward from the top of the disc
	SetShaderColor(0.12f, 0.10f, 0.08f, 1.0f);
	SetTransformations(glm::vec3(0.07f, 0.6f, 0.07f), 0, 0, 0, glm::vec3(-9.6f, 9.4f, -4.5f));
	m_basicMeshes->DrawBoxMesh();

	// Pan 2 — body (smaller radius, mounted below Pan 1)
	SetShaderColor(0.15f, 0.12f, 0.10f, 1.0f);
	SelectMaterial("shiny");
	SetTransformations(glm::vec3(0.55f, 0.1f, 0.55f), 0, 0, 90.0f, glm::vec3(-9.6f, 7.0f, -2.8f));
	m_basicMeshes->DrawCylinderMesh();

	// Pan 2 — handle
	SetShaderColor(0.12f, 0.10f, 0.08f, 1.0f);
	SetTransformations(glm::vec3(0.07f, 0.5f, 0.07f), 0, 0, 0, glm::vec3(-9.6f, 7.75f, -2.8f));
	m_basicMeshes->DrawBoxMesh();
}

void SceneManager::RenderSink()
{
	// Cabinet carcass — wood texture on a box proportioned to a
	SelectMaterial("standard");
	SetShaderTexture(gTextureWood, glm::vec2(1.0f, 1.0f));
	SetTransformations(glm::vec3(3.0f, 3.5f, 5.0f), 0, 0, 0, glm::vec3(-8.4f, 1.75f, -3.5f));
	m_basicMeshes->DrawBoxMesh();

	// Marble countertop — slightly oversized relative to the cabinet
	SelectMaterial("shiny");
	SetShaderTexture(gTextureMarble, glm::vec2(1.0f, 1.0f));
	SetTransformations(glm::vec3(3.1f, 0.2f, 5.1f), 0, 0, 0, glm::vec3(-8.4f, 3.5f, -3.5f));
	m_basicMeshes->DrawBoxMesh();

	// Tap spout
	SetShaderColor(0.75f, 0.75f, 0.75f, 1.0f);
	SetTransformations(glm::vec3(0.06f, 0.5f, 0.06f), 0, 0, 0, glm::vec3(-9.4f, 3.75f, -3.5f));
	m_basicMeshes->DrawCylinderMesh();

	// Cold-water valve handle — blue convention.
	SetShaderColor(0.2f, 0.2f, 0.8f, 1.0f);
	SetTransformations(glm::vec3(0.15f, 0.15f, 0.15f), 0, 0, 0, glm::vec3(-9.0f, 3.575f, -4.2f));
	m_basicMeshes->DrawBoxMesh();

	// Hot-water valve handle — red convention.
	SetShaderColor(0.8f, 0.2f, 0.2f, 1.0f);
	SetTransformations(glm::vec3(0.15f, 0.15f, 0.15f), 0, 0, 0, glm::vec3(-9.0f, 3.575f, -2.8f));
	m_basicMeshes->DrawBoxMesh();

	// Waste bin — cylindrical body resting on the floor
	SelectMaterial("standard");
	SetShaderColor(0.9f, 0.9f, 0.9f, 1.0f);
	SetTransformations(glm::vec3(0.8f, 2.0f, 0.8f), 0, 0, 0, glm::vec3(-8.5f, 0.0f, -7.5f));
	m_basicMeshes->DrawCylinderMesh();

	// Waste bin lid — dark disc capping the cylinder.
	SetShaderColor(0.2f, 0.2f, 0.2f, 1.0f);
	SetTransformations(glm::vec3(0.85f, 0.1f, 0.85f), 0, 0, 0, glm::vec3(-8.5f, 2.0f, -7.5f));
	m_basicMeshes->DrawCylinderMesh();

	// --- DRAWER KNOBS (side by side on top drawer) ---
	SetShaderColor(0.7f, 0.55f, 0.1f, 1.0f); // brass
	SetTransformations(glm::vec3(0.06f, 0.06f, 0.06f), 0, 0, 0, glm::vec3(-6.9f, 2.8f, -2.8f));
	m_basicMeshes->DrawSphereMesh();

	SetShaderColor(0.7f, 0.55f, 0.1f, 1.0f);
	SetTransformations(glm::vec3(0.06f, 0.06f, 0.06f), 0, 0, 0, glm::vec3(-6.9f, 2.8f, -4.2f));
	m_basicMeshes->DrawSphereMesh();

	// --- LEFT CABINET DOOR PANEL ---
	SetShaderColor(0.75f, 0.62f, 0.45f, 1.0f); // lighter wood inset
	SetTransformations(glm::vec3(0.05f, 1.8f, 1.6f), 0, 0, 0, glm::vec3(-6.9f, 1.2f, -2.8f));
	m_basicMeshes->DrawBoxMesh();

	// Left door handle
	SetShaderColor(0.7f, 0.55f, 0.1f, 1.0f);
	SetTransformations(glm::vec3(0.05f, 0.35f, 0.05f), 0, 0, 0, glm::vec3(-6.9f, 1.2f, -2.3f));
	m_basicMeshes->DrawCylinderMesh();

	// --- RIGHT CABINET DOOR PANEL ---
	SetShaderColor(0.75f, 0.62f, 0.45f, 1.0f);
	SetTransformations(glm::vec3(0.05f, 1.8f, 1.6f), 0, 0, 0, glm::vec3(-6.9f, 1.2f, -4.2f));
	m_basicMeshes->DrawBoxMesh();

	// Right door handle
	SetShaderColor(0.7f, 0.55f, 0.1f, 1.0f);
	SetTransformations(glm::vec3(0.05f, 0.35f, 0.05f), 0, 0, 0, glm::vec3(-6.9f, 1.2f, -4.7f));
	m_basicMeshes->DrawCylinderMesh();

	// --- DRAWER FACE (thin box above the two doors) ---
	SetShaderColor(0.80f, 0.68f, 0.50f, 1.0f); // slightly lighter than cabinet
	SetTransformations(glm::vec3(0.05f, 0.5f, 3.8f), 0, 0, 0, glm::vec3(-6.9f, 2.8f, -3.5f));
	m_basicMeshes->DrawBoxMesh();
}
void SceneManager::RenderWindow()
{
	// --- LIGHTING OCCLUSION MASK ---
	SetShaderColor(0.01f, 0.01f, 0.01f, 1.0f);
	SetTransformations(glm::vec3(4.25f, 4.25f, 0.01f), 0.0f, 0.0f, 0.0f, glm::vec3(4.0f, 5.75f, -9.9f));
	m_basicMeshes->DrawBoxMesh();

	// --- FRAME ASSEMBLY ---
	SetShaderColor(0.3f, 0.15f, 0.05f, 1.0f);
	SetTransformations(glm::vec3(4.5f, 0.15f, 0.4f), 0.0f, 0.0f, 0.0f, glm::vec3(4.0f, 3.5f, -9.7f));
	m_basicMeshes->DrawBoxMesh();

	SetTransformations(glm::vec3(0.15f, 4.5f, 0.15f), 0.0f, 0.0f, 0.0f, glm::vec3(1.85f, 5.75f, -9.7f));
	m_basicMeshes->DrawBoxMesh();

	SetTransformations(glm::vec3(0.15f, 4.5f, 0.15f), 0.0f, 0.0f, 0.0f, glm::vec3(6.15f, 5.75f, -9.7f));
	m_basicMeshes->DrawBoxMesh();

	SetTransformations(glm::vec3(4.5f, 0.15f, 0.15f), 0.0f, 0.0f, 0.0f, glm::vec3(4.0f, 8.0f, -9.7f));
	m_basicMeshes->DrawBoxMesh();

	SetTransformations(glm::vec3(0.1f, 4.5f, 0.1f), 0.0f, 0.0f, 0.0f, glm::vec3(4.0f, 5.75f, -9.7f));
	m_basicMeshes->DrawBoxMesh();

	// --- GLASS SURFACE ---
	// Alpha blending implementation for transparent geometry.
	glDepthMask(GL_FALSE);
	SetShaderColor(0.9f, 0.95f, 1.0f, 0.15f);
	SetTransformations(glm::vec3(4.2f, 4.2f, 0.01f), 0.0f, 0.0f, 0.0f, glm::vec3(4.0f, 5.75f, -9.8f));
	m_basicMeshes->DrawBoxMesh();
	glDepthMask(GL_TRUE);

}