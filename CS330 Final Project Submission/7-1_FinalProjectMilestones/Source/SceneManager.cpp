///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Nathan Hunter
//	Created for CS-330-Computational Graphics and Visualization, 8/23/26
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"
#include <chrono>

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

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
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
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
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
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

	modelView = translation * rotationX * rotationY * rotationZ * scale;

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
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
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

/***********************************************************
* DefineObjectMaterials()
 *
* This method is used for configuring the various material
* se�ngs for all of the objects in the 3D scene.
***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	float lightColorR = 200.0f / 255.0f;
	float lightColorG = 200.0f / 255.0f;
	float lightColorB = 200.0f / 255.0f;
	float brightness = 0.5f;

	OBJECT_MATERIAL metalMaterial;
	metalMaterial.ambientColor = glm::vec3(lightColorR * brightness, lightColorG * brightness, lightColorB * brightness);
	metalMaterial.ambientStrength = 0.05f;
	metalMaterial.diffuseColor = glm::vec3(lightColorR * brightness, lightColorG * brightness, lightColorB * brightness);
	metalMaterial.specularColor = glm::vec3(lightColorR * brightness, lightColorG * brightness, lightColorB * brightness);
	metalMaterial.shininess = 20.0;
	metalMaterial.tag = "metal";
	m_objectMaterials.push_back(metalMaterial);

	OBJECT_MATERIAL clothMaterial;
	clothMaterial.ambientColor = glm::vec3(lightColorR * brightness, lightColorG * brightness, lightColorB * brightness);
	clothMaterial.ambientStrength = 0.05f;
	clothMaterial.diffuseColor = glm::vec3(lightColorR * 0.3, lightColorG * 0.3, lightColorB * 0.3);
	clothMaterial.specularColor = glm::vec3(1, 1, 1);
	clothMaterial.shininess = 0.0;
	clothMaterial.tag = "cloth";
	m_objectMaterials.push_back(clothMaterial);

	OBJECT_MATERIAL plasticMaterial;
	plasticMaterial.ambientColor = glm::vec3(lightColorR * brightness, lightColorG * brightness, lightColorB * brightness);
	plasticMaterial.ambientStrength = 0.05f;
	plasticMaterial.diffuseColor = glm::vec3(lightColorR * 0.3, lightColorG * 0.3, lightColorB * 0.3);
	plasticMaterial.specularColor = glm::vec3(1, 1, 1);
	plasticMaterial.shininess = 1.0;
	plasticMaterial.tag = "plastic";
	m_objectMaterials.push_back(plasticMaterial);


}

void SceneManager::SetupSceneLights()
{
	m_pShaderManager->setBoolValue("bUseLighting", true);

	float light0ColorR = 255.0f / 255.0f;
	float light0ColorG = 205.0f / 255.0f;
	float light0ColorB = 128.0f / 255.0f;
	float light0Strength = 0.75f;

	

	// morning sunlight
	m_pShaderManager->setVec3Value("lightSources[0].position", -10.0f, 100.0f, 20.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", light0ColorR * light0Strength, light0ColorG * light0Strength, light0ColorB * light0Strength);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", light0ColorR * light0Strength, light0ColorG * light0Strength, light0ColorB * light0Strength);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", light0ColorR * light0Strength, light0ColorG * light0Strength, light0ColorB * light0Strength);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 0.15f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.2f);

	// fountain light
	float light1ColorR = 0.0f / 255.0f;
	float light1ColorG = 0.0f / 255.0f;
	float light1ColorB = 255.0f / 255.0f;
	float light1Strength = 1.0f;
	m_pShaderManager->setVec3Value("lightSources[1].position", 2.9f, 0.75f, 3.0f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", light1ColorR * light1Strength, light1ColorG * light1Strength, light1ColorB * light1Strength);
	
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
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	CreateGLTexture("Textures/water.jpg", "water");
	CreateGLTexture("Textures/metal.jpg", "metal");
	CreateGLTexture("Textures/carpet.jpg", "carpet");
	CreateGLTexture("Textures/hay.jpg", "hay");
	CreateGLTexture("Textures/cloth.jpg", "cloth");
	BindGLTextures();
	
	DefineObjectMaterials();
	SetupSceneLights();

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadSphereMesh();

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
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(10.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("carpet");
	SetShaderMaterial("cloth");
	// draw the mesh with transformation values and texture/material values set above
	m_basicMeshes->DrawPlaneMesh();

	// draw full fountain object
	DrawFountain(4.0f, 0.75f, 3.0f);

	// draw full bunny object
	DrawBunny(-2.0f, 0.25f, 3.0f);

	// draw full feeder object
	DrawFeeder(3.5f, 1.5f, -0.5f);

	// draw full storage box object
	DrawStorageBox(-3.5f, 1.5f, -0.5f);

	/****************************************************************/
}

void SceneManager::DrawFountain(float posX, float posY, float posZ) 
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// base color for fountain object
	const float BASE_COLOR_R = 138.0f / 255.0f;
	const float BASE_COLOR_G = 143.0f / 255.0f;
	const float BASE_COLOR_B = 150.0f / 255.0f;
	SetShaderColor(BASE_COLOR_R, BASE_COLOR_G, BASE_COLOR_B, 1.0f);
	SetShaderMaterial("metal");

	// base of the fountain
	float fountainBaseHeight = 1.25f;
	scaleXYZ = glm::vec3(2.0f, fountainBaseHeight, 3.0f);
	positionXYZ = glm::vec3(
		posX + 0.0f,
		posY + 0.0f,
		posZ + 0.0f);
	SetTransformations(scaleXYZ,XrotationDegrees,YrotationDegrees,ZrotationDegrees,positionXYZ);
	m_basicMeshes->DrawBoxMesh();

	// base of the spout
	float spoutHeight = 0.3f;
	scaleXYZ = glm::vec3(0.1f, spoutHeight, 0.1f);
	positionXYZ = glm::vec3(
		posX + 0.0f,
		posY + fountainBaseHeight/2,
		posZ + 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	m_basicMeshes->DrawCylinderMesh();

	// top of the spout
	float spoutLength = 0.9f;
	scaleXYZ = glm::vec3(0.12f, spoutLength, 0.12f);
	positionXYZ = glm::vec3(
		posX,
		posY + fountainBaseHeight/2 + spoutHeight,
		posZ - spoutLength / 2);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees + 90, ZrotationDegrees + 90, positionXYZ);
	m_basicMeshes->DrawCylinderMesh();

	// blue light on the base
	scaleXYZ = glm::vec3(0.3f, 0.3f, 0.3f);
	positionXYZ = glm::vec3(
		posX - 1.0f,
		posY,
		posZ);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees + 90, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.3f, 0.3f, 1.0f, 1.0f);
	m_basicMeshes->DrawTorusMesh();

	// pool of water
	SetShaderMaterial("cloth");
	SetShaderTexture("water");

	scaleXYZ = glm::vec3(0.9f, 0, 1.4f);
	positionXYZ = glm::vec3(
		posX + 0.0f,
		posY + 0.64f,
		posZ + 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	m_basicMeshes->DrawPlaneMesh();

	// flowing water
	// get time for flowing animation
	auto now = std::chrono::system_clock::now();
	auto duration = now.time_since_epoch();
	auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
	long long ms_int = millis.count();


	float flowDistance = 0.9f;
	// the flow distance is animated by using the current time in milliseconds to create a repeating pattern
	scaleXYZ = glm::vec3(flowDistance + (ms_int/100 % 3)/10.0f, flowDistance / 2, flowDistance / 6);
	positionXYZ = glm::vec3(
		posX + 0.0f,
		posY + fountainBaseHeight/2 - 0.12,
		posZ + 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees + 90, ZrotationDegrees, positionXYZ);
	m_basicMeshes->DrawTorusMesh();

	

}

void SceneManager::DrawBunny(float posX, float posY, float posZ) {

	SetShaderMaterial("cloth");
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 45.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;
	const float BUNNY_WIDTH = 1.25f;
	SetShaderColor(97.0f / 255.0f, 70.0f / 255.0f, 23.0f / 255.0f, 1.0f);


	// base of the bunny
	scaleXYZ = glm::vec3(BUNNY_WIDTH, 2.0f, BUNNY_WIDTH);
	positionXYZ = glm::vec3(
	posX + 0.0f,
	posY + 1.25f,
	posZ + 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees - 45.0f, positionXYZ);
	m_basicMeshes->DrawSphereMesh();


	// foot 1
	scaleXYZ = glm::vec3(2.0f, 0.5f, 0.5f);
	positionXYZ = glm::vec3(
		posX - BUNNY_WIDTH / 2,
		posY,
		posZ - BUNNY_WIDTH / 2);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	m_basicMeshes->DrawBoxMesh();

	// foot 2
	scaleXYZ = glm::vec3(2.0f, 0.5f, 0.5f);
	positionXYZ = glm::vec3(
		posX + BUNNY_WIDTH / 2.0f,
		posY,
		posZ + BUNNY_WIDTH / 2.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	m_basicMeshes->DrawBoxMesh();

	const float FRONT_LEG_OFFSET = 0.8f;
	// front leg 1
	scaleXYZ = glm::vec3(2.0f, 0.5f, 0.5f);
	positionXYZ = glm::vec3(
		posX - BUNNY_WIDTH / 2.2f + FRONT_LEG_OFFSET,
		posY + 0.60f,
		posZ - BUNNY_WIDTH / 2.2f - FRONT_LEG_OFFSET);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees - 45.0f, positionXYZ);
	m_basicMeshes->DrawBoxMesh();

	// front leg 2
	scaleXYZ = glm::vec3(2.0f, 0.5f, 0.5f);
	positionXYZ = glm::vec3(
		posX + BUNNY_WIDTH / 2.2f + FRONT_LEG_OFFSET,
		posY + 0.60f,
		posZ + BUNNY_WIDTH / 2.2f - FRONT_LEG_OFFSET);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees - 45.0f, positionXYZ);
	m_basicMeshes->DrawBoxMesh();

	const float HEAD_OFFSET = 1.3f;
	const float HEAD_BODY_RATIO = 0.7f;
	// head
	scaleXYZ = glm::vec3(1.1f, BUNNY_WIDTH * HEAD_BODY_RATIO, BUNNY_WIDTH * HEAD_BODY_RATIO);
	positionXYZ = glm::vec3(
		posX + HEAD_OFFSET,
		posY + 2.2f,
		posZ - HEAD_OFFSET);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees - 10.0f, positionXYZ);
	m_basicMeshes->DrawSphereMesh();

	const float EAR_OFFSET = 0.8f;
	// ear 1
	scaleXYZ = glm::vec3(0.4f, 1.0f, 0.4f);
	positionXYZ = glm::vec3(
		posX - (BUNNY_WIDTH / 2.0f) * HEAD_BODY_RATIO + HEAD_OFFSET * EAR_OFFSET,
		posY + 3.2f,
		posZ - (BUNNY_WIDTH / 2.0f) * HEAD_BODY_RATIO - HEAD_OFFSET * EAR_OFFSET);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees + 20.0f, positionXYZ);
	m_basicMeshes->DrawSphereMesh();

	// ear 2
	scaleXYZ = glm::vec3(0.4f, 1.0f, 0.4f);
	positionXYZ = glm::vec3(
		posX + (BUNNY_WIDTH / 2.0f) * HEAD_BODY_RATIO + HEAD_OFFSET * EAR_OFFSET,
		posY + 3.2f,
		posZ + (BUNNY_WIDTH / 2.0f) * HEAD_BODY_RATIO - HEAD_OFFSET * EAR_OFFSET);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees + 20.0f, positionXYZ);
	m_basicMeshes->DrawSphereMesh();

}

void SceneManager::DrawFeeder(float posX, float posY, float posZ)
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// base color for feeder object
	const float BASE_COLOR_R = 200.0f / 255.0f;
	const float BASE_COLOR_G = 200.0f / 255.0f;
	const float BASE_COLOR_B = 200.0f / 255.0f;

	// secondary color for feeder object
	const float SECONDARY_COLOR_R = 20.0f / 255.0f;
	const float SECONDARY_COLOR_G = 20.0f / 255.0f;
	const float SECONDARY_COLOR_B = 20.0f / 255.0f;

	SetShaderColor(BASE_COLOR_R, BASE_COLOR_G, BASE_COLOR_B, 1.0f);
	SetShaderMaterial("plastic");

	const float FEEDER_WIDTH = 2.2f;
	const float FEEDER_HEIGHT = 3.0f;

	// base of the feeder
	scaleXYZ = glm::vec3(FEEDER_WIDTH, FEEDER_HEIGHT, FEEDER_WIDTH);
	positionXYZ = glm::vec3(
		posX + 0.0f,
		posY + 0.0f,
		posZ + 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	m_basicMeshes->DrawBoxMesh();

	const float TROUGH_ANGLE = 20.0f;
	const float TROUGH_ANGLE_SIN = sin(glm::radians(TROUGH_ANGLE));

	// edge 1 of the trough
	scaleXYZ = glm::vec3(FEEDER_WIDTH / 2.0f - TROUGH_ANGLE_SIN * FEEDER_WIDTH / 6.0f, 0, FEEDER_HEIGHT / 6.0f);
	positionXYZ = glm::vec3(
		posX - FEEDER_WIDTH + TROUGH_ANGLE_SIN * (FEEDER_WIDTH / 6.0f),
		posY - FEEDER_HEIGHT / 3.0f,
		posZ + (FEEDER_WIDTH / 2.0f + 0.01));
	SetTransformations(scaleXYZ, XrotationDegrees + 90.0f, YrotationDegrees, ZrotationDegrees, positionXYZ);
	m_basicMeshes->DrawPlaneMesh();

	// edge 2 of the trough
	scaleXYZ = glm::vec3(FEEDER_WIDTH / 2.0f - TROUGH_ANGLE_SIN * FEEDER_WIDTH / 6.0f, 0, FEEDER_HEIGHT / 6.0f);
	positionXYZ = glm::vec3(
		posX - FEEDER_WIDTH + TROUGH_ANGLE_SIN * (FEEDER_WIDTH / 6.0f),
		posY - FEEDER_HEIGHT / 3.0f,
		posZ - (FEEDER_WIDTH / 2.0f + 0.01));
	SetTransformations(scaleXYZ, XrotationDegrees + 90.0f, YrotationDegrees, ZrotationDegrees, positionXYZ);
	m_basicMeshes->DrawPlaneMesh();

	// edge 3 of the trough
	scaleXYZ = glm::vec3(FEEDER_HEIGHT / 6.0f, 0, FEEDER_WIDTH / 2.0f);
	positionXYZ = glm::vec3(
		posX - FEEDER_WIDTH - (FEEDER_WIDTH / 2.0f - TROUGH_ANGLE_SIN * FEEDER_WIDTH / 6.0f) + 0.15f,
		posY - FEEDER_HEIGHT / 3.0f,
		posZ);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees + 90.0f, positionXYZ);
	SetShaderColor(BASE_COLOR_R, BASE_COLOR_G, BASE_COLOR_B, 1.0f);
	m_basicMeshes->DrawPlaneMesh();

	// food inside of the holder
	scaleXYZ = glm::vec3(FEEDER_WIDTH * 0.9f, FEEDER_HEIGHT / 2.0f * 0.9f, FEEDER_WIDTH * 0.9f);
	positionXYZ = glm::vec3(
		posX + 0.0f,
		posY + FEEDER_HEIGHT - FEEDER_HEIGHT / 4.0f + 0.01,
		posZ + 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("cloth");
	SetShaderTexture("hay");
	m_basicMeshes->DrawBoxMesh();

	// food holder of the feeder
	scaleXYZ = glm::vec3(FEEDER_WIDTH, FEEDER_HEIGHT / 2.0f, FEEDER_WIDTH);
	positionXYZ = glm::vec3(
		posX + 0.0f,
		posY + FEEDER_HEIGHT - FEEDER_HEIGHT / 4.0f + 0.01,
		posZ + 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(SECONDARY_COLOR_R, SECONDARY_COLOR_G, SECONDARY_COLOR_B, 0.9f);
	SetShaderMaterial("plastic");
	m_basicMeshes->DrawBoxMesh();

	// trough of the feeder base
	scaleXYZ = glm::vec3(FEEDER_WIDTH - 0.09f, FEEDER_HEIGHT / 3.0f, FEEDER_WIDTH - 0.09f);
	positionXYZ = glm::vec3(
		posX - FEEDER_WIDTH + TROUGH_ANGLE_SIN * (FEEDER_HEIGHT / 3.0f),
		posY - FEEDER_HEIGHT / 3 - TROUGH_ANGLE_SIN * (FEEDER_HEIGHT / 3.0f),
		posZ + 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees + TROUGH_ANGLE, positionXYZ);
	SetShaderMaterial("cloth");
	SetShaderTexture("hay");
	m_basicMeshes->DrawBoxMesh();

}

void SceneManager::DrawStorageBox(float posX, float posY, float posZ)
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// base texture for storage box object
	SetShaderMaterial("cloth");
	SetShaderTexture("cloth");

	const float STORAGE_BOX_WIDTH = 3.0f;
	const float STORAGE_BOX_HEIGHT = 3.0f;

	// base of the storage box
	scaleXYZ = glm::vec3(STORAGE_BOX_WIDTH, STORAGE_BOX_HEIGHT, STORAGE_BOX_WIDTH);
	positionXYZ = glm::vec3(
		posX + 0.0f,
		posY + 0.0f,
		posZ + 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	m_basicMeshes->DrawBoxMesh();

	// lid of the storage box
	scaleXYZ = glm::vec3(STORAGE_BOX_WIDTH * 1.2f, STORAGE_BOX_HEIGHT / 10.0f, STORAGE_BOX_WIDTH * 1.2f);
	positionXYZ = glm::vec3(
		posX + 0.0f,
		posY + STORAGE_BOX_HEIGHT / 2.0f,
		posZ + 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	m_basicMeshes->DrawBoxMesh();
}