///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ================
// This file contains the implementation of the `SceneManager` class, which is 
// responsible for managing the preparation and rendering of 3D scenes. It 
// handles textures, materials, lighting configurations, and object rendering.
//
// AUTHOR: Brian Battersby
// INSTITUTION: Southern New Hampshire University (SNHU)
// COURSE: CS-330 Computational Graphics and Visualization
//
// INITIAL VERSION: November 1, 2023
// LAST REVISED: December 1, 2024
//
// RESPONSIBILITIES:
// - Load, bind, and manage textures in OpenGL.
// - Define materials and lighting properties for 3D objects.
// - Manage transformations and shader configurations.
// - Render complex 3D scenes using basic meshes.
//
// NOTE: This implementation leverages external libraries like `stb_image` for 
// texture loading and GLM for matrix and vector operations.
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}
//prepareScene
/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/


//*************************************************************
/*
This method draws a box
Parameter 1: glm::vec3 scaleXYZ ---- format ---- glm::vec3(7.0f, 2.5f, 2.0f)
Parameter 2: float XrotationDegrees
Parameter 3: float YrotationDegrees
Parameter 4: float ZrotationDegrees
Parameter 5: glm::vec3 positionXYZ ---- format ---- glm::vec3(0.0f, 1.25f, 0.0f)
Parameter 6: std::string texture 
Parameter 7: std::string material	^^^shaders material
This method returns nothing
*/
void SceneManager::drawBox(glm::vec3 scale,
	float Xrotation,
	float Yrotation,
	float Zrotation,
	glm::vec3 position,
	std::string texture,
	std::string material) {
	// set the XYZ scale for the Box
	glm::vec3 scaleXYZ = scale;

	// set the XYZ rotation for the mesh
	float XrotationDegrees = Xrotation;
	float YrotationDegrees = Yrotation;
	float ZrotationDegrees = Zrotation;

	// set the XYZ position for the mesh
	glm::vec3 positionXYZ = position;

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.0f, 0.0f, 0.0f, 1);
	SetShaderTexture(texture);
	SetShaderMaterial(material);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


}
/*
This method draws a cylinder
Parameter 1: glm::vec3 scaleXYZ ---- format ---- glm::vec3(7.0f, 2.5f, 2.0f)
Parameter 2: float XrotationDegrees
Parameter 3: float YrotationDegrees
Parameter 4: float ZrotationDegrees
Parameter 5: glm::vec3 positionXYZ ---- format ---- glm::vec3(0.0f, 1.25f, 0.0f)
Parameter 6: std::string texture
Parameter 7: std::string material	^^^shaders material
This method returns nothing
*/
//***************************************************************************************
void SceneManager::drawCylinder(glm::vec3 scale,
	float Xrotation,
	float Yrotation,
	float Zrotation,
	glm::vec3 position,
	std::string texture,
	std::string material) {
	// set the XYZ scale for the Cylinder
	glm::vec3 scaleXYZ = scale;

	// set the XYZ rotation for the mesh
	float XrotationDegrees = Xrotation;
	float YrotationDegrees = Yrotation;
	float ZrotationDegrees = Zrotation;

	// set the XYZ position for the mesh
	glm::vec3 positionXYZ = position;

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.0f, 0.0f, 0.0f, 1);
	SetShaderTexture(texture);
	SetShaderMaterial(material);
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
}
/*
This method draws a Prism
Parameter 1: glm::vec3 scaleXYZ ---- format ---- glm::vec3(7.0f, 2.5f, 2.0f)
Parameter 2: float XrotationDegrees
Parameter 3: float YrotationDegrees
Parameter 4: float ZrotationDegrees
Parameter 5: glm::vec3 positionXYZ ---- format ---- glm::vec3(0.0f, 1.25f, 0.0f)
Parameter 6: std::string texture
Parameter 7: std::string material	^^^shaders material
This method returns nothing
*/
//******************************************************************************************
void SceneManager::drawPrism(glm::vec3 scale,
	float Xrotation,
	float Yrotation,
	float Zrotation,
	glm::vec3 position,
	std::string texture,
	std::string material) {
	// set the XYZ scale for the Prism
	glm::vec3 scaleXYZ = scale;

	// set the XYZ rotation for the mesh
	float XrotationDegrees = Xrotation;
	float YrotationDegrees = Yrotation;
	float ZrotationDegrees = Zrotation;

	// set the XYZ position for the mesh
	glm::vec3 positionXYZ = position;

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.0f, 0.0f, 0.0f, 1);
	SetShaderTexture(texture);
	SetShaderMaterial(material);
	// draw the mesh with transformation values
	m_basicMeshes->DrawPrismMesh();
}
/*
This method draws a Sphere
Parameter 1: glm::vec3 scaleXYZ ---- format ---- glm::vec3(7.0f, 2.5f, 2.0f)
Parameter 2: float XrotationDegrees
Parameter 3: float YrotationDegrees
Parameter 4: float ZrotationDegrees
Parameter 5: glm::vec3 positionXYZ ---- format ---- glm::vec3(0.0f, 1.25f, 0.0f)
Parameter 6: std::string texture
Parameter 7: std::string material	^^^shaders material
This method returns nothing
*/
//******************************************************************************************
void SceneManager::drawSphere(glm::vec3 scale,
	float Xrotation,
	float Yrotation,
	float Zrotation,
	glm::vec3 position,
	std::string texture,
	std::string material) {
	// set the XYZ scale for the Prism
	glm::vec3 scaleXYZ = scale;

	// set the XYZ rotation for the mesh
	float XrotationDegrees = Xrotation;
	float YrotationDegrees = Yrotation;
	float ZrotationDegrees = Zrotation;

	// set the XYZ position for the mesh
	glm::vec3 positionXYZ = position;

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.0f, 0.0f, 0.0f, 1);
	SetShaderTexture(texture);
	SetShaderMaterial(material);
	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();
}
/*
This method draws a plane mesh
Parameter 1: glm::vec3 scaleXYZ ---- format ---- glm::vec3(7.0f, 2.5f, 2.0f)
Parameter 2: float XrotationDegrees
Parameter 3: float YrotationDegrees
Parameter 4: float ZrotationDegrees
Parameter 5: glm::vec3 positionXYZ ---- format ---- glm::vec3(0.0f, 1.25f, 0.0f)
Parameter 6: std::string texture	
Parameter 7: std::string material	^^^shaders material
This method returns nothing
*/
//******************************************************************************************
void SceneManager::drawPlane(glm::vec3 scale,
	float Xrotation,
	float Yrotation,
	float Zrotation,
	glm::vec3 position,
	std::string texture,
	std::string material) {
	// set the XYZ scale for the Prism
	glm::vec3 scaleXYZ = scale;

	// set the XYZ rotation for the mesh
	float XrotationDegrees = Xrotation;
	float YrotationDegrees = Yrotation;
	float ZrotationDegrees = Zrotation;

	// set the XYZ position for the mesh
	glm::vec3 positionXYZ = position;

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.0f, 0.0f, 0.0f, 1);
	SetShaderTexture(texture);
	SetShaderMaterial(material);
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
}
/*
This method draws a plane mesh
Parameter 1: glm::vec3 scaleXYZ ---- format ---- glm::vec3(7.0f, 2.5f, 2.0f)
Parameter 2: float XrotationDegrees
Parameter 3: float YrotationDegrees
Parameter 4: float ZrotationDegrees
Parameter 5: glm::vec3 positionXYZ ---- format ---- glm::vec3(0.0f, 1.25f, 0.0f)
Parameter 6: std::string texture
Parameter 7: std::string material	^^^shaders material
This method returns nothing
*/
//******************************************************************************************
void SceneManager::drawCone(glm::vec3 scale,
	float Xrotation,
	float Yrotation,
	float Zrotation,
	glm::vec3 position,
	std::string texture,
	std::string material) {

	// set the XYZ scale for the Prism
	glm::vec3 scaleXYZ = scale;

	// set the XYZ rotation for the mesh
	float XrotationDegrees = Xrotation;
	float YrotationDegrees = Yrotation;
	float ZrotationDegrees = Zrotation;

	// set the XYZ position for the mesh
	glm::vec3 positionXYZ = position;

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.0f, 0.0f, 0.0f, 1);
	SetShaderTexture(texture);
	SetShaderMaterial(material);
	// draw the mesh with transformation values
	//m_basicMeshes->DrawConeMesh();
	m_basicMeshes->DrawConeMesh();
	
}
void SceneManager::drawTorus(glm::vec3 scale,
	float Xrotation,
	float Yrotation,
	float Zrotation,
	glm::vec3 position,
	std::string texture,
	std::string material) {

	// set the XYZ scale for the Prism
	glm::vec3 scaleXYZ = scale;

	// set the XYZ rotation for the mesh
	float XrotationDegrees = Xrotation;
	float YrotationDegrees = Yrotation;
	float ZrotationDegrees = Zrotation;

	// set the XYZ position for the mesh
	glm::vec3 positionXYZ = position;

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.0f, 0.0f, 0.0f, 1);
	SetShaderTexture(texture);
	SetShaderMaterial(material);
	// draw the mesh with transformation values
	//m_basicMeshes->DrawConeMesh();
	m_basicMeshes->DrawTorusMesh();
}

//******************************************************************************************
/*
This method draws the entire coffin object and is implemented to make the code easier to read, maintain and more modular
This method returns nothing
*/
void SceneManager::drawCoffin() {
	//main body of the coffin
	//					Scale vector		XYZ roation				Position vector		texture			Lighting
	//drawBox(glm::vec3(7.0f, 2.5f, 2.0f), 0.0, 7.0, 0.0, glm::vec3(0.0f, 1.25f, 0.0f), "coffin");
	//drawBox(glm::vec3(7.0f, 2.5f, 2.0f), 0.0, -7.0, 0.0, glm::vec3(0.0f, 1.25f, 1.0f), "coffin");
	//drawBox(glm::vec3(2.12f, 2.5f, 2.12f), 0.0, 90.0, 0.0, glm::vec3(-2.55f, 1.25f, 0.5f), "coffin");

	//top
	drawBox(glm::vec3(2.12f, 2.5f, 2.12f), 0.0, -7.0, 0.0, glm::vec3(4.55f, 1.25f, -0.2f), "coffin", "coffin");
	drawBox(glm::vec3(2.12f, 2.5f, 2.12f), 0.0, 7.0, 0.0, glm::vec3(4.55f, 1.25f, 1.2f), "coffin", "coffin");
	drawBox(glm::vec3(2.12f, 2.5f, 3.25f), 0.0, 0.0, 0.0, glm::vec3(4.7f, 1.25f, 0.5f), "coffin", "coffin");
	//middle
	drawBox(glm::vec3(4.24f, 2.5f, 3.75f), 0.0, 0.0, 0.0, glm::vec3(1.5f, 1.25f, 0.5f), "coffin", "coffin");

	//bottom
	drawBox(glm::vec3(2.12f, 2.5f, 2.12f), 0.0, 7.0, 0.0, glm::vec3(-1.5f, 1.25f, -0.2f), "coffin", "coffin");
	drawBox(glm::vec3(2.12f, 2.5f, 2.12f), 0.0, -7.0, 0.0, glm::vec3(-1.5f, 1.25f, 1.2f), "coffin", "coffin");
	drawBox(glm::vec3(2.12f, 2.5f, 3.25f), 0.0, 0.0, 0.0, glm::vec3(-1.65f, 1.25f, 0.5f), "coffin", "coffin");

	//drawPrism(glm::vec3(3.15f, 2.5f, 2.15f), 0.0, -90.0, 0.0, glm::vec3(4.95f, 1.25f, 0.5), "coffin");
	 
	//Coffin horizontal rim
	//						Scale vector		XYZ roation				Position vector		texture			Lighting
	//sides
	drawCylinder(glm::vec3(0.10f, 4.15f, 0.10f), 90.0, 90.0, 0.0, glm::vec3(-0.55f, 2.5f, 2.35f), "rim", "coffinRim");
	drawCylinder(glm::vec3(0.10f, 4.15f, 0.10f), 90.0, 90.0, 0.0, glm::vec3(-0.55f, 2.5f, -1.37f), "rim", "coffinRim");
	//front side
	drawCylinder(glm::vec3(0.10f, 2.12f, 0.10f), 90.0, -83.0, 0.0, glm::vec3(-0.55f, 2.5f, -1.37f), "rim", "coffinRim");
	drawCylinder(glm::vec3(0.10f, 2.12f, 0.10f), 90.0, -97.0, 0.0, glm::vec3(-0.55f, 2.5f, 2.35f), "rim", "coffinRim");
	//back side
	drawCylinder(glm::vec3(0.10f, 2.12f, 0.10f), 90.0, -97.0, 0.0, glm::vec3(5.7f, 2.5f, -1.1f), "rim", "coffinRim");
	drawCylinder(glm::vec3(0.10f, 2.12f, 0.10f), 90.0, -83.0, 0.0, glm::vec3(5.7f, 2.5f, 2.1f), "rim", "coffinRim");
	//top and bottom
	drawCylinder(glm::vec3(0.10f, 3.25f, 0.10f), 90.0, 0.0, 0.0, glm::vec3(5.7f, 2.5f, -1.13f), "rim", "coffinRim");
	drawCylinder(glm::vec3(0.10f, 3.25f, 0.10f), 90.0, 0.0, 0.0, glm::vec3(-2.7f, 2.5f, -1.13f), "rim", "coffinRim");



	//Coffin spheres
	//						Scale vector		XYZ roation				Position vector		texture		Lighting
	//front side
	drawSphere(glm::vec3(0.15f, 0.15f, 0.15f), 0.0, 0.0, 0.0, glm::vec3(3.6f, 2.5f, -1.37), "rim", "coffinRim");
	drawSphere(glm::vec3(0.15f, 0.15f, 0.15f), 0.0, 0.0, 0.0, glm::vec3(3.6f, 2.5f, 2.35), "rim", "coffinRim");
	//back side
	drawSphere(glm::vec3(0.15f, 0.15f, 0.15f), 0.0, 0.0, 0.0, glm::vec3(-0.55f, 2.5f, -1.37), "rim", "coffinRim");
	drawSphere(glm::vec3(0.15f, 0.15f, 0.15f), 0.0, 0.0, 0.0, glm::vec3(-0.55f, 2.5f, 2.35), "rim", "coffinRim");
	//front
	drawSphere(glm::vec3(0.15f, 0.15f, 0.15f), 0.0, 0.0, 0.0, glm::vec3(5.7f, 2.5f, -1.12), "rim", "coffinRim");
	drawSphere(glm::vec3(0.15f, 0.15f, 0.15f), 0.0, 0.0, 0.0, glm::vec3(5.7f, 2.5f, 2.1), "rim", "coffinRim");
	//back
	drawSphere(glm::vec3(0.15f, 0.15f, 0.15f), 0.0, 0.0, 0.0, glm::vec3(-2.7f, 2.5f, -1.12), "rim", "coffinRim");
	drawSphere(glm::vec3(0.15f, 0.15f, 0.15f), 0.0, 0.0, 0.0, glm::vec3(-2.7f, 2.5f, 2.1), "rim", "coffinRim");


	//Coffin vertical rim
	//						Scale vector		XYZ roation				Position vector		texture		Lighting
	//front side
	drawCylinder(glm::vec3(0.10f, 2.5f, 0.10f), 0.0, 0.0, 0.0, glm::vec3(-0.55f, 0.0f, -1.37), "rim", "coffinRim");
	drawCylinder(glm::vec3(0.10f, 2.5f, 0.10f), 0.0, 0.0, 0.0, glm::vec3(-0.55f, 0.0f, 2.35), "rim", "coffinRim");

	//back side side
	drawCylinder(glm::vec3(0.10f, 2.5f, 0.10f), 0.0, 0.0, 0.0, glm::vec3(3.6f, 0.0f, -1.37), "rim", "coffinRim");
	drawCylinder(glm::vec3(0.10f, 2.5f, 0.10f), 0.0, 0.0, 0.0, glm::vec3(3.6f, 0.0f, 2.35), "rim", "coffinRim");

	//front
	drawCylinder(glm::vec3(0.10f, 2.5f, 0.10f), 0.0, 0.0, 0.0, glm::vec3(5.7f, 0.0f, -1.12), "rim", "coffinRim");
	drawCylinder(glm::vec3(0.10f, 2.5f, 0.10f), 0.0, 0.0, 0.0, glm::vec3(5.7f, 0.0f, 2.1), "rim", "coffinRim");

	//back
	drawCylinder(glm::vec3(0.10f, 2.5f, 0.10f), 0.0, 0.0, 0.0, glm::vec3(-2.7f, 0.0f, -1.12), "rim", "coffinRim");
	drawCylinder(glm::vec3(0.10f, 2.5f, 0.10f), 0.0, 0.0, 0.0, glm::vec3(-2.7f, 0.0f, 2.1), "rim", "coffinRim");

	/* 
	//Coffin horizontal rim
	//					Scale vector				XYZ roation				Position vector		texture
	drawCylinder(glm::vec3(0.10f, 7.03f, 0.10f), 90.0, -97.0, 0.0, glm::vec3(3.39f, 2.5f, 2.41f), "rim");
	drawCylinder(glm::vec3(0.10f, 2.65f, 0.10f), 90.0, -83.0, 0.0, glm::vec3(6.02f, 2.5f, 2.11f), "rim");
	drawCylinder(glm::vec3(0.10f, 7.0f, 0.10f), 90.0, -83.0, 0.0, glm::vec3(3.35f, 2.5f, -1.4f), "rim");
	drawCylinder(glm::vec3(0.10f, 2.65f, 0.10f), 90.0, -97.0, 0.0, glm::vec3(6.02f, 2.5f, -1.07f), "rim");
	drawCylinder(glm::vec3(0.10f, 3.15f, 0.10f), 90.0, 0.0, 0.0, glm::vec3(6.02f, 2.5f, -1.03f), "rim");
	drawCylinder(glm::vec3(0.10f, 2.02f, 0.10f), 90.0, 0.0, 0.0, glm::vec3(-3.6f, 2.5f, -0.5f), "rim");
	//Coffin vertical rim
	//						Scale vector		XYZ roation				Position vector		texture
	drawCylinder(glm::vec3(0.10f, 2.5f, 0.10f), 0.0, 0.0, 0.0, glm::vec3(3.35f, 0.0f, 2.43), "rim");

	drawCylinder(glm::vec3(0.10f, 2.5f, 0.10f), 0.0, 0.0, 0.0, glm::vec3(6.02f, 0.0f, 2.1), "rim");

	drawCylinder(glm::vec3(0.10f, 2.5f, 0.10f), 0.0, 0.0, 0.0, glm::vec3(3.35f, 0.0f, -1.4), "rim");

	drawCylinder(glm::vec3(0.10f, 2.5f, 0.10f), 0.0, 0.0, 0.0, glm::vec3(6.02f, 0.0f, -1.05f), "rim");
	drawCylinder(glm::vec3(0.10f, 2.5f, 0.10f), 0.0, 0.0, 0.0, glm::vec3(-3.6f, 0.0f, 1.5f), "rim");
	drawCylinder(glm::vec3(0.10f, 2.5f, 0.10f), 0.0, 0.0, 0.0, glm::vec3(-3.6f, 0.0f, -0.5f), "rim");
	
	//Coffin corner spheres
	//						Scale vector		XYZ roation				Position vector		texture
	drawSphere(glm::vec3(0.15f, 0.15f, 0.15f), 0.0, 0.0, 0.0, glm::vec3(3.35f, 2.5f, 2.43), "rim");

	drawSphere(glm::vec3(0.15f, 0.15f, 0.15f), 0.0, 0.0, 0.0, glm::vec3(6.02f, 2.5f, 2.1), "rim");

	drawSphere(glm::vec3(0.15f, 0.15f, 0.15f), 0.0, 0.0, 0.0, glm::vec3(3.35f, 2.5f, -1.4), "rim");

	drawSphere(glm::vec3(0.15f, 0.15f, 0.15f), 0.0, 0.0, 0.0, glm::vec3(6.02f, 2.5f, -1.05), "rim");
	drawSphere(glm::vec3(0.15f, 0.15f, 0.15f), 0.0, 0.0, 0.0, glm::vec3(-3.6f, 2.5f, 1.5), "rim");
	drawSphere(glm::vec3(0.15f, 0.15f, 0.15f), 0.0, 0.0, 0.0, glm::vec3(-3.6f, 2.5f, -0.5), "rim");
	*/
}

//******************************************************************************************
/*
This method draws the entire book case object and is implemented to make the code easier to read, maintain and more modular
This method returns nothing
*/
void SceneManager::drawBookCase() {
	//					Scale vector		XYZ roation				Position vector		texture		Lighting
	// back of book case
	drawBox(glm::vec3(8.0f, 5.0f, 0.3f), 0.0, 0.0, 0.0, glm::vec3(-2.0f, 2.5f, -9.8f), "bookCase", "bookCase");
	// right side of the book case
	drawBox(glm::vec3(2.0f, 5.0f, 0.3f), 0.0, 90.0, 0.0, glm::vec3(2.1f, 2.5f, -9.0f), "bookCase", "bookCase");
	// left side of the book case
	drawBox(glm::vec3(2.0f, 5.0f, 0.3f), 0.0, 90.0, 0.0, glm::vec3(-6.1f, 2.5f, -9.0f), "bookCase", "bookCase");
	// bottom of the book case
	drawBox(glm::vec3(2.0f, 8.0f, 0.3f), 90.0, 90.0, 0.0, glm::vec3(-2.0f, 0.2f, -9.0f), "bookCase", "bookCase");
	// top of the book case
	drawBox(glm::vec3(2.0f, 8.5f, 0.3f), 90.0, 90.0, 0.0, glm::vec3(-2.0f, 5.0f, -9.0f), "bookCase", "bookCase");
	// top and bottom divider of the book case
	drawBox(glm::vec3(2.0f, 8.5f, 0.3f), 90.0, 90.0, 0.0, glm::vec3(-2.0f, 2.0f, -9.0f), "bookCase", "bookCase");
	drawBox(glm::vec3(2.0f, 8.5f, 0.3f), 90.0, 90.0, 0.0, glm::vec3(-2.0f, 3.5f, -9.0f), "bookCase", "bookCase");
	// middle of the book case
	drawBox(glm::vec3(2.0f, 5.0f, 0.3f), 0.0, 90.0, 0.0, glm::vec3(-2.0f, 2.3f, -9.0f), "bookCase", "bookCase");

	//books******************************************************************************************************
	//					Scale vector		XYZ roation				Position vector		texture		Lighting
	//Top left
	drawBox(glm::vec3(0.8f, 1.3f, 0.5f), 0.0, 90.0, 0.0, glm::vec3(-5.5f, 4.15f, -8.6f), "book4", "coffin");
	drawBox(glm::vec3(0.8f, 1.3f, 0.5f), 0.0, 90.0, 0.0, glm::vec3(-5.0f, 4.15f, -8.6f), "book6", "coffin");
	drawBox(glm::vec3(0.8f, 1.0f, 0.5f), 0.0, 90.0, 0.0, glm::vec3(-4.5f, 4.15f, -8.6f), "book2", "coffin");
	drawBox(glm::vec3(0.8f, 1.3f, 0.5f), 0.0, 90.0, 0.0, glm::vec3(-4.0f, 4.15f, -8.6f), "book", "coffin");
	drawBox(glm::vec3(0.8f, 1.0f, 0.5f), 0.0, 90.0, 0.0, glm::vec3(-3.5f, 4.15f, -8.6f), "book5", "coffin");
	drawBox(glm::vec3(0.8f, 1.0f, 0.5f), 0.0, 90.0, 0.0, glm::vec3(-3.0f, 4.15f, -8.6f), "book2", "coffin");
	drawBox(glm::vec3(0.8f, 1.0f, 0.5f), 0.0, 90.0, 0.0, glm::vec3(-2.5f, 4.15f, -8.6f), "book3", "coffin");
	//top right
	//					Scale vector		XYZ roation				Position vector		texture		Lighting
	drawBox(glm::vec3(0.8f, 1.0f, 0.5f), 0.0, 90.0, 0.0, glm::vec3(-1.5f, 4.15f, -8.6f), "book2", "coffin");
	drawBox(glm::vec3(0.8f, 1.0f, 0.5f), 0.0, 90.0, 0.0, glm::vec3(-1.0f, 4.15f, -8.6f), "book2", "coffin");
	drawBox(glm::vec3(0.8f, 1.3f, 0.5f), 0.0, 90.0, 0.0, glm::vec3(-0.5f, 4.15f, -8.6f), "book4", "coffin");
	drawBox(glm::vec3(0.8f, 1.0f, 0.5f), 0.0, 90.0, 0.0, glm::vec3(0.0f, 4.15f, -8.6f), "book5", "coffin");
	drawBox(glm::vec3(0.8f, 1.3f, 0.5f), 0.0, 90.0, 0.0, glm::vec3(0.5f, 4.15f, -8.6f), "book6", "coffin");
	drawBox(glm::vec3(0.8f, 1.3f, 0.5f), 0.0, 90.0, 0.0, glm::vec3(1.0f, 4.15f, -8.6f), "book", "coffin");
	drawBox(glm::vec3(0.8f, 1.0f, 0.5f), 0.0, 90.0, 0.0, glm::vec3(1.5f, 4.15f, -8.6f), "book3", "coffin");
	//middle left
	//					Scale vector		XYZ roation				Position vector		texture		Lighting
	drawBox(glm::vec3(0.8f, 1.3f, 0.5f), 0.0, 90.0, 0.0, glm::vec3(-5.5f, 2.65f, -8.6f), "book", "coffin");
	drawBox(glm::vec3(0.8f, 1.0f, 0.5f), 0.0, 90.0, 0.0, glm::vec3(-5.0f, 2.65f, -8.6f), "book2", "coffin");
	drawBox(glm::vec3(0.8f, 1.0f, 0.5f), 0.0, 90.0, 0.0, glm::vec3(-4.5f, 2.65f, -8.6f), "book3", "coffin");
	drawBox(glm::vec3(0.8f, 1.3f, 0.5f), 0.0, 90.0, 0.0, glm::vec3(-4.0f, 2.65f, -8.6f), "book4", "coffin");
	drawBox(glm::vec3(0.8f, 1.0f, 0.5f), 0.0, 90.0, 0.0, glm::vec3(-3.5f, 2.65f, -8.6f), "book5", "coffin");
	drawBox(glm::vec3(0.8f, 1.3f, 0.5f), 0.0, 90.0, 0.0, glm::vec3(-3.0f, 2.65f, -8.6f), "book6", "coffin");
	drawBox(glm::vec3(0.8f, 1.0f, 0.5f), 0.0, 90.0, 0.0, glm::vec3(-2.5f, 2.65f, -8.6f), "book2", "coffin");
	//middle right
	//					Scale vector		XYZ roation				Position vector		texture		Lighting
	drawBox(glm::vec3(0.8f, 1.0f, 0.5f), 0.0, 90.0, 0.0, glm::vec3(-1.5f, 2.65f, -8.6f), "book3", "coffin");
	drawBox(glm::vec3(0.8f, 1.3f, 0.5f), 0.0, 90.0, 0.0, glm::vec3(-1.0f, 2.65f, -8.6f), "book6", "coffin");
	drawBox(glm::vec3(0.8f, 1.3f, 0.5f), 0.0, 90.0, 0.0, glm::vec3(-0.5f, 2.65f, -8.6f), "book4", "coffin");
	drawBox(glm::vec3(0.8f, 1.0f, 0.5f), 0.0, 90.0, 0.0, glm::vec3(0.0f, 2.65f, -8.6f), "book", "coffin");
	drawBox(glm::vec3(0.8f, 1.0f, 0.5f), 0.0, 90.0, 0.0, glm::vec3(0.5f, 2.65f, -8.6f), "book2", "coffin");
	drawBox(glm::vec3(0.8f, 1.0f, 0.5f), 0.0, 90.0, 0.0, glm::vec3(1.0f, 2.65f, -8.6f), "book5", "coffin");
	drawBox(glm::vec3(0.8f, 1.3f, 0.5f), 0.0, 90.0, 0.0, glm::vec3(1.5f, 2.65f, -8.6f), "book2", "coffin");
	//bottom left
	//					Scale vector		XYZ roation				Position vector		texture		Lighting
	drawBox(glm::vec3(0.8f, 1.3f, 0.5f), 0.0, 90.0, 0.0, glm::vec3(-5.5f, 0.8f, -8.6f), "book6", "coffin");
	drawBox(glm::vec3(0.8f, 1.3f, 0.5f), 0.0, 90.0, 0.0, glm::vec3(-5.0f, 0.8f, -8.6f), "book4", "coffin");
	drawBox(glm::vec3(0.8f, 1.0f, 0.5f), 0.0, 90.0, 0.0, glm::vec3(-4.5f, 0.8f, -8.6f), "book5", "coffin");
	drawBox(glm::vec3(0.8f, 1.0f, 0.5f), 0.0, 90.0, 0.0, glm::vec3(-4.0f, 0.8f, -8.6f), "book2", "coffin");
	drawBox(glm::vec3(0.8f, 1.0f, 0.5f), 0.0, 90.0, 0.0, glm::vec3(-3.5f, 0.8f, -8.6f), "book3", "coffin");
	drawBox(glm::vec3(0.8f, 1.3f, 0.5f), 0.0, 90.0, 0.0, glm::vec3(-3.0f, 0.8f, -8.6f), "book", "coffin");
	drawBox(glm::vec3(0.8f, 1.0f, 0.5f), 0.0, 90.0, 0.0, glm::vec3(-2.5f, 0.8f, -8.6f), "book2", "coffin");
	//bottom right
	//					Scale vector		XYZ roation				Position vector		texture		Lighting
	drawBox(glm::vec3(0.8f, 1.3f, 0.5f), 0.0, 90.0, 0.0, glm::vec3(-1.5f, 0.8f, -8.6f), "book4", "coffin");
	drawBox(glm::vec3(0.8f, 1.0f, 0.5f), 0.0, 90.0, 0.0, glm::vec3(-1.0f, 0.8f, -8.6f), "book5", "coffin");
	drawBox(glm::vec3(0.8f, 1.0f, 0.5f), 0.0, 90.0, 0.0, glm::vec3(-0.5f, 0.8f, -8.6f), "book3", "coffin");
	drawBox(glm::vec3(0.8f, 1.0f, 0.5f), 0.0, 90.0, 0.0, glm::vec3(0.0f, 0.8f, -8.6f), "book2", "coffin");
	drawBox(glm::vec3(0.8f, 1.0f, 0.5f), 0.0, 90.0, 0.0, glm::vec3(0.5f, 0.8f, -8.6f), "book2", "coffin");
	drawBox(glm::vec3(0.8f, 1.3f, 0.5f), 0.0, 90.0, 0.0, glm::vec3(1.0f, 0.8f, -8.6f), "book6", "coffin");
	drawBox(glm::vec3(0.8f, 1.3f, 0.5f), 0.0, 90.0, 0.0, glm::vec3(1.5f, 0.8f, -8.6f), "book", "coffin");
	//books****************************************************************************************************


}
//******************************************************************************************
/*
This method draws the entire book case object and is implemented to make the code easier to read, maintain and more modular
This method returns nothing
*/
void SceneManager::drawSword() {

	//Sword pommel
	//						Scale vector		XYZ roation				Position vector		texture		Lighting

	drawSphere(glm::vec3(0.5f,0.5f, 0.5f), 0.0, 0.0, 0.0, glm::vec3(-6.0f, 6.0f, -9.3), "pommel", "coffinRim");

	drawCylinder(glm::vec3(0.2f, 2.0f, 0.2f), 0.0, 0.0, -60.0, glm::vec3(-5.7f, 6.1f, -9.3), "pommel", "coffinRim");

	drawBox(glm::vec3(0.2f, 2.0f, 0.2f), 0.0, 0.0, 30.0, glm::vec3(-4.05f, 7.05f, -9.3), "pommel", "coffinRim");

	//Blade
	//						Scale vector		XYZ roation				Position vector		texture		Lighting
	drawPlane(glm::vec3(3.0f, 0.0f, 0.2f), 90.0, 0.0, 30.0, glm::vec3(-1.5f, 8.5f, -9.3), "blade", "blade");
	drawPrism(glm::vec3(0.4f, 0.01f, 0.4f), 90.0, 0.0, 120.0, glm::vec3(1.27f, 10.1f, -9.31), "blade", "blade");
}
/*
This method draws the entire candle stand object and is implemented to make the code easier to read, maintain and more modular
This method returns nothing
*/
void SceneManager::drawCandle() {

	//base of the candle holder
	drawCone(glm::vec3(0.5f, 1.5f, 0.5f), 0.0, 0.0, 0.0, glm::vec3(6.0f, 0.5f, 6.0), "pommel", "coffinRim");
	drawCylinder(glm::vec3(0.05f, 4.0f, 0.05f), 0.0, 0.0, 0.0, glm::vec3(6.0f, 1.5f, 6.0), "pommel", "coffinRim");
	drawTorus(glm::vec3(0.05f, 0.05f, 0.05f), 90.0, 0.0, 0.0, glm::vec3(6.0f, 1.85f, 6.0), "pommel", "coffinRim");
	//legs
	drawBox(glm::vec3(0.05f, 0.7f, 0.05f), 0.0, 0.0, -20.0, glm::vec3(5.45f, 0.2f, 6.0), "pommel", "coffinRim");
	drawBox(glm::vec3(0.05f, 0.7f, 0.05f), -20.0, 0.0, 0.0, glm::vec3(6.0f, 0.2f, 6.55), "pommel", "coffinRim");
	drawBox(glm::vec3(0.05f, 0.7f, 0.05f), 20.0, 0.0, 15.0, glm::vec3(6.3f, 0.2f, 5.45f), "pommel", "coffinRim");
	//side arms
	//horizontal
	drawCylinder(glm::vec3(0.05f, 1.0f, 0.05f), 90.0, 0.0, 0.0, glm::vec3(6.0f, 4.5f, 6.0), "pommel", "coffinRim");
	drawCylinder(glm::vec3(0.05f, 1.0f, 0.05f), -90.0, 0.0, 0.0, glm::vec3(6.0f, 4.5f, 6.0), "pommel", "coffinRim");
	//vertical
	drawCylinder(glm::vec3(0.05f, 0.8f, 0.05f), 0.0, 0.0, 0.0, glm::vec3(6.0f, 4.45f, 5.0), "pommel", "coffinRim");
	drawCylinder(glm::vec3(0.05f, 0.8f, 0.05f), 0.0, 0.0, 0.0, glm::vec3(6.0f, 4.45f, 7.0), "pommel", "coffinRim");

	//candle base
	drawCylinder(glm::vec3(0.3f, 0.02f, 0.3f), 0.0, 0.0, 0.0, glm::vec3(6.0f, 5.25f, 7.0), "pommel", "coffinRim");
	drawCylinder(glm::vec3(0.3f, 0.02f, 0.3f), 0.0, 0.0, 0.0, glm::vec3(6.0f, 5.25f, 5.0), "pommel", "coffinRim");
	drawCylinder(glm::vec3(0.3f, 0.02f, 0.3f), 0.0, 0.0, 0.0, glm::vec3(6.0f, 5.5f, 6.0), "pommel", "coffinRim");
	//candle base rim
	drawTorus(glm::vec3(0.3f, 0.3f, 0.3f), 90.0, 0.0, 0.0, glm::vec3(6.0f, 5.25f, 7.0), "pommel", "coffinRim");
	drawTorus(glm::vec3(0.3f, 0.3f, 0.3f), 90.0, 0.0, 0.0, glm::vec3(6.0f, 5.25f, 5.0), "pommel", "coffinRim");
	drawTorus(glm::vec3(0.3f, 0.3f, 0.3f), 90.0, 0.0, 0.0, glm::vec3(6.0f, 5.5f, 6.0), "pommel", "coffinRim");
	//candles
	drawCylinder(glm::vec3(0.1f, 0.6f, 0.1f), 0.0, 0.0, 0.0, glm::vec3(6.0f, 5.25f, 7.0), "candle", "candle");
	drawCylinder(glm::vec3(0.1f, 0.6f, 0.1f), 0.0, 0.0, 0.0, glm::vec3(6.0f, 5.25f, 5.0), "candle", "candle");
	drawCylinder(glm::vec3(0.15f, 0.65f, 0.15f), 0.0, 0.0, 0.0, glm::vec3(6.0f, 5.5f, 6.0), "candle", "candle");

	//candle wick
	drawCone(glm::vec3(0.05f, 0.3f, 0.05f), 0.0, 0.0, 0.0, glm::vec3(6.0f, 5.8f, 7.0), "wick", "candle");
	drawCone(glm::vec3(0.05f, 0.3f, 0.05f), 0.0, 0.0, 0.0, glm::vec3(6.0f, 5.8f, 5.0), "wick", "candle");
	drawCone(glm::vec3(0.05f, 0.3f, 0.05f), 0.0, 0.0, 0.0, glm::vec3(6.0f, 6.1f, 6.0), "wick", "candle");


	

	
}
/*
This method draws the floor and wall and is implemented to make the code easier to read, maintain and more modular
This method returns nothing
*/
void SceneManager::drawWallFloor() {
	//Draw floor plane
	//						Scale vector		XYZ roation				Position vector		texture		Lighting
	drawPlane(glm::vec3(20.0f, 1.0f, 15.0f), 0.0, 0.0, 0.0, glm::vec3(0.0f, 0.0f, 0.0f), "brickWall", "floor");
	//Draw the back wall
	drawPlane(glm::vec3(20.0f, 3.0f, 10.0f), 90.0, 0.0, 0.0, glm::vec3(0.0f, 7.0f, -10.0f), "wall", "wall");
	//Draw the side wall
	drawPlane(glm::vec3(15.0f, 1.0f, 10.0f), 90.0, 90.0, 0.0, glm::vec3(20.0f, 7.0f, 0.0f), "wall", "wall");
}


/***********************************************************
 *  LoadSceneTextures()
 *
 *  This method is used for loading the textures into the
 *  the project
 *
 ***********************************************************/
void SceneManager::LoadSceneTextures() {
	bool bReturn = false;
	//brick wall
	bReturn = CreateGLTexture("textures/brick.jpg", "brick");
	//brick floor
	bReturn = CreateGLTexture("textures/brickWall.jpg", "brickWall");

	bReturn = CreateGLTexture("textures/coffin.jpg", "coffin");

	bReturn = CreateGLTexture("textures/rim.jpg", "rim");

	bReturn = CreateGLTexture("textures/wall.jpg", "wall");

	bReturn = CreateGLTexture("textures/book1.png", "book");

	bReturn = CreateGLTexture("textures/bookCase.jpg", "bookCase");

	bReturn = CreateGLTexture("textures/book2.png", "book2");

	bReturn = CreateGLTexture("textures/book3.png", "book3");

	bReturn = CreateGLTexture("textures/book4.png", "book4");

	bReturn = CreateGLTexture("textures/book5.png", "book5");

	bReturn = CreateGLTexture("textures/book6.png", "book6");

	bReturn = CreateGLTexture("textures/pommel.jpg", "pommel");

	bReturn = CreateGLTexture("textures/blade.jpg", "blade");

	bReturn = CreateGLTexture("textures/candle.jpg", "candle");

	bReturn = CreateGLTexture("textures/wick.jpg", "wick");

	BindGLTextures();
}


/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is used to set the lighting in the scene
 ***********************************************************/
void SceneManager::SetupSceneLights() {
	// Enable lighting in the shader
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	/*
	// Directional lighting
	//from the top
	m_pShaderManager->setVec3Value("directionalLight.direction", 0.0f, -1.0f, 0.0f);
	m_pShaderManager->setVec3Value("directionalLight.ambient", 0.6f, 0.6f, 0.6f);
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 0.9f, 0.9f, 0.9f);
	m_pShaderManager->setVec3Value("directionalLight.specular", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);
	*/

	// Point light 1
	m_pShaderManager->setVec3Value("pointLights[0].position", 5.0f, 5.0f, 6.0);
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.9f, 0.9f, 0.9f);
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setFloatValue("pointLights[0].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[0].linear", 0.09f);
	m_pShaderManager->setFloatValue("pointLights[0].quadratic", 0.032f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

	
	// Point light 2
	m_pShaderManager->setVec3Value("pointLights[1].position", 7.0f, 5.0f, 8.0);
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 0.9f, 0.9f, 0.9f);
	m_pShaderManager->setVec3Value("pointLights[1].specular", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setFloatValue("pointLights[1].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[1].linear", 0.09f);
	m_pShaderManager->setFloatValue("pointLights[1].quadratic", 0.032f);
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);
	
	// Point light 3
	m_pShaderManager->setVec3Value("pointLights[2].position", -6.0f, 6.0f, -7.0);
	m_pShaderManager->setVec3Value("pointLights[2].ambient", 0.6f, 0.6f, 0.6f);
	m_pShaderManager->setVec3Value("pointLights[2].diffuse", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setVec3Value("pointLights[2].specular", 0.6f, 0.6f, 0.6f);
	m_pShaderManager->setFloatValue("pointLights[2].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[2].linear", 0.09f);
	m_pShaderManager->setFloatValue("pointLights[2].quadratic", 0.032f);
	m_pShaderManager->setBoolValue("pointLights[2].bActive", true);
	
	// Directional lighting 
	//from the front
	m_pShaderManager->setVec3Value("directionalLight.direction", 0.0f, 0.0f, -1.0f);
	m_pShaderManager->setVec3Value("directionalLight.ambient", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("directionalLight.specular", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);
	/*
	// Directional lighting #3
//from the left
	m_pShaderManager->setVec3Value("directionalLight.direction", 1.0f, 0.0f, 0.0f);
	m_pShaderManager->setVec3Value("directionalLight.ambient", 0.6f, 0.6f, 0.6f);
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 0.9f, 0.9f, 0.9f);
	m_pShaderManager->setVec3Value("directionalLight.specular", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);
	*/
}
/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used to apply material lighting to enhance the shaders
 ***********************************************************/
void SceneManager::DefineObjectMaterials() {
	//Lighting for the walls
	OBJECT_MATERIAL wall;
	wall.diffuseColor = glm::vec3(0.0f, 0.0f, 0.0f);
	wall.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	wall.shininess = 1.0;
	wall.tag = "wall";

	m_objectMaterials.push_back(wall);
	//Lighting for the floor
	OBJECT_MATERIAL floor;
	floor.diffuseColor = glm::vec3(0.0f, 0.0f, 0.0f);
	floor.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	floor.shininess = 1.0;
	floor.tag = "floor";

	m_objectMaterials.push_back(floor);
	//Lighting for the main coffin body
	OBJECT_MATERIAL coffin;
	coffin.diffuseColor = glm::vec3(0.0f, 0.0f, 0.0f);
	coffin.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	coffin.shininess = 1.0;
	coffin.tag = "coffin";

	m_objectMaterials.push_back(coffin);
	//Lighting for the coffin rim
	OBJECT_MATERIAL coffinRim;
	coffinRim.diffuseColor = glm::vec3(0.01f, 0.01f, 0.01f);
	coffinRim.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	coffinRim.shininess = 100.0;
	coffinRim.tag = "coffinRim";

	m_objectMaterials.push_back(coffinRim);

	//Lighting for the book case
	OBJECT_MATERIAL bookCase;
	bookCase.diffuseColor = glm::vec3(0.0f, 0.0f, 0.0f);
	bookCase.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	bookCase.shininess = 1.0;
	bookCase.tag = "bookCase";

	m_objectMaterials.push_back(bookCase);

	//Lighting for the sword blade
	OBJECT_MATERIAL blade;
	blade.diffuseColor = glm::vec3(0.0f, 0.0f, 0.0f);
	blade.specularColor = glm::vec3(0.9f, 0.9f, 0.9f);
	blade.shininess = 32.0;
	blade.tag = "blade";

	m_objectMaterials.push_back(blade);

	//Lighting for the candle
	OBJECT_MATERIAL candle;
	candle.diffuseColor = glm::vec3(0.9f, 0.9f, 0.9f);
	candle.specularColor = glm::vec3(0.9f, 0.9f, 0.9f);
	candle.shininess = 32.0;
	candle.tag = "candle";

	m_objectMaterials.push_back(candle);
}
/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	//render lighting
	SetupSceneLights();
	//apply object lighting
	DefineObjectMaterials();
	//apply textures to objects
	LoadSceneTextures();
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadPrismMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadExtraTorusMesh1();
	m_basicMeshes->LoadExtraTorusMesh2();
}




/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/


	//Draw the wall and the floor
	drawWallFloor();
	//*************************************************************************
	//Draw the entire coffin object 
	drawCoffin();
	//*************************************************************************
	//Draw book case object
	drawBookCase();
	//*************************************************************************
	//Draw the sword
	drawSword();
	//*************************************************************************
	//Draw the candel holder object
	drawCandle();
}
