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
	m_loadedTextures = 0;
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

// Load Scene Texture Functionality
void SceneManager::LoadSceneTextures()
{
	CreateGLTexture("textures/magenta_concrete.png", "pconcrete");
	CreateGLTexture("textures/pinkandblue/ChristmasTreeOrnament019.png", "pinkandblue");
	CreateGLTexture("textures/lapis_block.png", "lapis");
	CreateGLTexture("textures/stainless.jpg", "stainless");
	CreateGLTexture("textures/blue_concrete.png", "bconcrete");
	CreateGLTexture("textures/pink_metal.png", "pmetal");
	CreateGLTexture("textures/pink_fabric.png", "pfabric");
	CreateGLTexture("textures/red_fabric.jpg", "rfab");
	CreateGLTexture("textures/cast_iron.jpg", "ciron");
	CreateGLTexture("textures/dark_cyan_concrete.jpg", "dbconcrete");
	CreateGLTexture("textures/cyan_concrete.jpg", "cconcrete");
	CreateGLTexture("textures/cyan_shiplap.png", "cshiplap");
	CreateGLTexture("textures/cyanshiplap2.jpg", "cshiplap2");
	CreateGLTexture("textures/pb_gradient.jpg", "pbgradient");
	CreateGLTexture("textures/orange_yellow_gradient.jpg", "oygradient");
	CreateGLTexture("textures/blue_building.jpg", "bb");

	BindGLTextures();
}

//**********************************************************************************************
// Implement Lighting
// 
// object materials - darken scene to mimic darker alley vibe
void SceneManager::DefineObjectMaterials()
{
	m_pShaderManager->setVec3Value("material.ambientColor", 0.15f, 0.05f, 0.03f);
	m_pShaderManager->setVec3Value("material.diffuseColor", 0.2f, 0.4f, 0.9f);
	m_pShaderManager->setVec3Value("material.specularColor", 0.4f, 0.1f, 0.5f);
	m_pShaderManager->setFloatValue("material.shininess", 16.0f);
}

// implement double lighting
void SceneManager::SetupSceneLights()
{
	m_pShaderManager->setBoolValue(g_UseLightingName, true); // true/false boolean lighting toggle

	// Light 0 - magenta accent
	m_pShaderManager->setVec3Value("lightSources[0].position", -2.0f, 5.0f, 15.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.1f, 0.0f, 0.1f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.09f, 0.1f, 0.6f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 1.0f, 0.4f, 0.8f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 30.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.6f);

	// Light 1 - blue tint to mimic neo blue tokyo style art
	m_pShaderManager->setVec3Value("lightSources[1].position", 0.0f, 10.0f, 10.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.05f, 0.02f, 0.06f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.01f, 0.5f, 1.0f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.2f, 0.7f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 50.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.4f);
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

	// need to load textures
	LoadSceneTextures();

	// defien object materials
	DefineObjectMaterials();

	// setup scene lights
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
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


	/******************************************************************/
	// BASE PLANE
	// 
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 25.0f); // doubled base plane size

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

	//SetShaderColor(0.05, 0.14, 0.58, 1); // changed plane color to dark blueish
	//disable plane color

	SetShaderTexture("lapis");
	SetTextureUVScale(16.0f, 16.0f); // tiled texture using UV to increase the visual detail

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/


	/****************************************************************/
	// PLANE - BRICK TILE COLOR
	// 
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(9.4f, 1.0f, 8.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-11.0f, 0.01f, 4.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.14, 0.18, 0.92, 1.0); // changed plane color to neon blueish

	SetShaderTexture("bb");
	SetTextureUVScale(10.0f, 10.0f); // tiled texture using UV to increase the visual detail

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************************************/
	/****************************************************************/
	// PLANE - BRICK TILE COLOR
	// 
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 1.0f, 2.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.3f, 0.01f, -1.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.14, 0.18, 0.92, 1.0); // changed plane color to neon blueish

	SetShaderTexture("bb");
	SetTextureUVScale(1.0f, 4.0f); // tiled texture using UV to increase the visual detail

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************************************/
	/****************************************************************/
	// PLANE - BRICK TILE COLOR right building
	// 
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(9.4f, 1.0f, 13.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(12.5f, 0.01f, 11.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.14, 0.18, 0.92, 1.0); // changed plane color to neon blueish

	SetShaderTexture("bb");
	SetTextureUVScale(10.0f, 12.0f); // tiled texture using UV to increase the visual detail

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************************************/

	//***************************************************************************************
	// PLANE - BRICK TILE BLOCKS inside
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(9.0f, 1.0f, 9.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-11.0f, 0.02f, 11.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.5, 0.9, 1, 1); // changed plane color to neon blueish
	// Change plan color to texture
	SetShaderTexture("bb");
	SetTextureUVScale(32.0f, 16.0f); // tiled texture using UV to increase the visual detail

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/***************************************************************************************/


	/***************************************************************************************/
	{ //blocked stool to isolate from other stool copies 
	//
	// STOOL 1 FRONT LEFT CENTER
	//
		glm::vec3 stoolOffset1 = glm::vec3(-4.0f, 0.0f, 5.0f); // used to control offset of stool from origin position

		// Seat
		scaleXYZ = glm::vec3(0.55f, 0.10f, 0.55f);
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;
		positionXYZ = stoolOffset1 + glm::vec3(0.0f, 1.45f, 0.0f);

		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		// SetShaderColor(0.90f, 0.25f, 0.75f, 0.9f);
		// Color changed to texture
		{ // top of seat
			SetShaderTexture("pconcrete");
			SetTextureUVScale(15.0f, 1.0f);
			m_basicMeshes->DrawCylinderMesh(true, false, false);
		}
		{ // bottom of seat
			SetShaderTexture("pmetal");
			SetTextureUVScale(0.25f, 0.25f);
			m_basicMeshes->DrawCylinderMesh(false, true, false);
		}
		{ // side of seat
			SetShaderTexture("pmetal");
			SetTextureUVScale(20.0f, 0.25f);
			m_basicMeshes->DrawCylinderMesh(false, false, true);
		}

		// Legs
			// blanket variables for the all stool 1 legs; scale, position, and angle
		glm::vec3 legScale = glm::vec3(0.03f, 1.55f, 0.03f);
		float legY = 0.01f;
		float legAngle = 6.0f;

		// Front-right leg
		SetTransformations(
			legScale,
			-legAngle,
			0.0f,
			legAngle,
			stoolOffset1 + glm::vec3(0.44f, legY, 0.44f));
		//SetShaderColor(0.90f, 0.25f, 0.75f, 1.0f); // neopink
		SetShaderTexture("pmetal");
		SetTextureUVScale(0.5f, 0.5f);

		m_basicMeshes->DrawCylinderMesh();

		// Front-left leg
		SetTransformations(
			legScale,
			-legAngle,
			0.0f,
			-legAngle,
			stoolOffset1 + glm::vec3(-0.44f, legY, 0.44f));
		//SetShaderColor(0.90f, 0.25f, 0.75f, 1.0f); // neopink
		SetShaderTexture("pmetal");

		m_basicMeshes->DrawCylinderMesh();

		// Back-right leg
		SetTransformations(
			legScale,
			legAngle,
			0.0f,
			legAngle,
			stoolOffset1 + glm::vec3(0.44f, legY, -0.44f));
		//SetShaderColor(0.90f, 0.25f, 0.75f, 1.0f); // neopink
		SetShaderTexture("pmetal");

		m_basicMeshes->DrawCylinderMesh();

		// Back-left leg
		SetTransformations(
			legScale,
			legAngle,
			0.0f,
			-legAngle,
			stoolOffset1 + glm::vec3(-0.44f, legY, -0.44f));
		//SetShaderColor(0.90f, 0.25f, 0.75f, 1.0f); // neopink
		SetShaderTexture("pmetal");

		m_basicMeshes->DrawCylinderMesh();


		// Footrest made from cylinders
		//SetShaderColor(0.90f, 0.25f, 0.75f, 1.0f); // neopink
		SetShaderTexture("pmetal");



		// blanket variables for all stool 1 foot bars; spread, height, radius, and length
		float legSpread = 0.44f;
		float footBarY = 0.50f;
		float footBarRadius = 0.03f;
		float footBarLength = legSpread * 2.0f;

		// front bar: starts at right front leg, extends to left
		SetTransformations(
			glm::vec3(footBarRadius, footBarLength, footBarRadius),
			0.0f,
			0.0f,
			90.0f,
			stoolOffset1 + glm::vec3(legSpread, footBarY, legSpread));
		m_basicMeshes->DrawCylinderMesh();

		// back bar: starts at right back leg, extends to left
		SetTransformations(
			glm::vec3(footBarRadius, footBarLength, footBarRadius),
			0.0f,
			0.0f,
			90.0f,
			stoolOffset1 + glm::vec3(legSpread, footBarY, -legSpread));
		m_basicMeshes->DrawCylinderMesh();

		// right bar: starts at back right leg, extends to front
		SetTransformations(
			glm::vec3(footBarRadius, footBarLength, footBarRadius),
			90.0f,
			0.0f,
			0.0f,
			stoolOffset1 + glm::vec3(legSpread, footBarY, -legSpread));
		m_basicMeshes->DrawCylinderMesh();

		// left bar: starts at back left leg, extends to front
		SetTransformations(
			glm::vec3(footBarRadius, footBarLength, footBarRadius),
			90.0f,
			0.0f,
			0.0f,
			stoolOffset1 + glm::vec3(-legSpread, footBarY, -legSpread));
		m_basicMeshes->DrawCylinderMesh();
	}


	{
		//
		// STOOL 2 Front Left DUPLICATE OF STOOL 1
		//
		glm::vec3 stoolOffset2 = glm::vec3(-4.0f, 0.0f, 6.95f);

		// Seat

		scaleXYZ = glm::vec3(0.55f, 0.10f, 0.55f);
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;
		positionXYZ = stoolOffset2 + glm::vec3(0.0f, 1.45f, 0.0f);

		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		// HOLD SetShaderColor(0.90f, 0.25f, 0.75f, 0.9f);
		SetShaderTexture("pconcrete");
		SetTextureUVScale(1.0, 1.0);
		m_basicMeshes->DrawCylinderMesh();


		// Legs
		glm::vec3 legScale = glm::vec3(0.03f, 1.55f, 0.03f);
		float legY = 0.01f;
		float legAngle = 6.0f;
		SetShaderTexture("pmetal");
		SetTextureUVScale(0.5f, 0.5f);

		// Front-right leg
		SetTransformations(
			legScale,
			-legAngle,
			0.0f,
			legAngle,
			stoolOffset2 + glm::vec3(0.44f, legY, 0.44f));
		//HOLD SetShaderColor(0.90f, 0.25f, 0.75f, 1.0f);
		m_basicMeshes->DrawCylinderMesh();

		// Front-left leg
		SetTransformations(
			legScale,
			-legAngle,
			0.0f,
			-legAngle,
			stoolOffset2 + glm::vec3(-0.44f, legY, 0.44f));
		//HOLD SetShaderColor(0.90f, 0.25f, 0.75f, 1.0f);
		m_basicMeshes->DrawCylinderMesh();

		// Back-right leg
		SetTransformations(
			legScale,
			legAngle,
			0.0f,
			legAngle,
			stoolOffset2 + glm::vec3(0.44f, legY, -0.44f));
		//HOLD SetShaderColor(0.90f, 0.25f, 0.75f, 1.0f);
		m_basicMeshes->DrawCylinderMesh();

		// Back-left leg
		SetTransformations(
			legScale,
			legAngle,
			0.0f,
			-legAngle,
			stoolOffset2 + glm::vec3(-0.44f, legY, -0.44f));
		//HOLD SetShaderColor(0.90f, 0.25f, 0.75f, 1.0f);
		m_basicMeshes->DrawCylinderMesh();


		// Footrest made from cylinders
		//HOLD SetShaderColor(0.90f, 0.25f, 0.75f, 1.0f);
		SetShaderTexture("pmetal");

		float legSpread = 0.44f;
		float footBarY = 0.50f;
		float footBarRadius = 0.03f;
		float footBarLength = legSpread * 2.0f;

		// front bar: starts at right front leg, extends to left
		SetTransformations(
			glm::vec3(footBarRadius, footBarLength, footBarRadius),
			0.0f,
			0.0f,
			90.0f,
			stoolOffset2 + glm::vec3(legSpread, footBarY, legSpread));
		m_basicMeshes->DrawCylinderMesh();

		// back bar: starts at right back leg, extends to left
		SetTransformations(
			glm::vec3(footBarRadius, footBarLength, footBarRadius),
			0.0f,
			0.0f,
			90.0f,
			stoolOffset2 + glm::vec3(legSpread, footBarY, -legSpread));
		m_basicMeshes->DrawCylinderMesh();

		// right bar: starts at back right leg, extends to front
		SetTransformations(
			glm::vec3(footBarRadius, footBarLength, footBarRadius),
			90.0f,
			0.0f,
			0.0f,
			stoolOffset2 + glm::vec3(legSpread, footBarY, -legSpread));
		m_basicMeshes->DrawCylinderMesh();

		// left bar: starts at back left leg, extends to front
		SetTransformations(
			glm::vec3(footBarRadius, footBarLength, footBarRadius),
			90.0f,
			0.0f,
			0.0f,
			stoolOffset2 + glm::vec3(-legSpread, footBarY, -legSpread));
		m_basicMeshes->DrawCylinderMesh();
	}

	{
		//
		// STOOL 3 Front Left DUPLICATE OF STOOL 1
		//
		glm::vec3 stoolOffset3 = glm::vec3(-4.00f, 0.0f, 3.05f);

		// Seat

		scaleXYZ = glm::vec3(0.55f, 0.10f, 0.55f);
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;
		positionXYZ = stoolOffset3 + glm::vec3(0.0f, 1.45f, 0.0f);

		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		//HOLD SetShaderColor(0.90f, 0.25f, 0.75f, 0.9f);
		SetShaderTexture("pconcrete");
		SetTextureUVScale(1.0f, 1.0f);

		m_basicMeshes->DrawCylinderMesh();



		// Legs
		glm::vec3 legScale = glm::vec3(0.03f, 1.55f, 0.03f);
		float legY = 0.01f;
		float legAngle = 6.0f;
		SetShaderTexture("pmetal");

		// Front-right leg
		SetTransformations(
			legScale,
			-legAngle,
			0.0f,
			legAngle,
			stoolOffset3 + glm::vec3(0.44f, legY, 0.44f));
		//HOLD SetShaderColor(0.90f, 0.25f, 0.75f, 1.0f);
		m_basicMeshes->DrawCylinderMesh();

		// Front-left leg
		SetTransformations(
			legScale,
			-legAngle,
			0.0f,
			-legAngle,
			stoolOffset3 + glm::vec3(-0.44f, legY, 0.44f));
		//HOLD SetShaderColor(0.90f, 0.25f, 0.75f, 1.0f);
		m_basicMeshes->DrawCylinderMesh();

		// Back-right leg
		SetTransformations(
			legScale,
			legAngle,
			0.0f,
			legAngle,
			stoolOffset3 + glm::vec3(0.44f, legY, -0.44f));
		//HOLD SetShaderColor(0.90f, 0.25f, 0.75f, 1.0f);
		m_basicMeshes->DrawCylinderMesh();

		// Back-left leg
		SetTransformations(
			legScale,
			legAngle,
			0.0f,
			-legAngle,
			stoolOffset3 + glm::vec3(-0.44f, legY, -0.44f));
		//HOLD SetShaderColor(0.90f, 0.25f, 0.75f, 1.0f);
		m_basicMeshes->DrawCylinderMesh();


		// Footrest made from cylinders
		//HOLD SetShaderColor(0.90f, 0.25f, 0.75f, 1.0f);
		SetShaderTexture("pmetal");

		float legSpread = 0.44f;
		float footBarY = 0.50f;
		float footBarRadius = 0.03f;
		float footBarLength = legSpread * 2.0f;

		// front bar: starts at right front leg, extends to left
		SetTransformations(
			glm::vec3(footBarRadius, footBarLength, footBarRadius),
			0.0f,
			0.0f,
			90.0f,
			stoolOffset3 + glm::vec3(legSpread, footBarY, legSpread));
		m_basicMeshes->DrawCylinderMesh();

		// back bar: starts at right back leg, extends to left
		SetTransformations(
			glm::vec3(footBarRadius, footBarLength, footBarRadius),
			0.0f,
			0.0f,
			90.0f,
			stoolOffset3 + glm::vec3(legSpread, footBarY, -legSpread));
		m_basicMeshes->DrawCylinderMesh();

		// right bar: starts at back right leg, extends to front
		SetTransformations(
			glm::vec3(footBarRadius, footBarLength, footBarRadius),
			90.0f,
			0.0f,
			0.0f,
			stoolOffset3 + glm::vec3(legSpread, footBarY, -legSpread));
		m_basicMeshes->DrawCylinderMesh();

		// left bar: starts at back left leg, extends to front
		SetTransformations(
			glm::vec3(footBarRadius, footBarLength, footBarRadius),
			90.0f,
			0.0f,
			0.0f,
			stoolOffset3 + glm::vec3(-legSpread, footBarY, -legSpread));
		m_basicMeshes->DrawCylinderMesh();
	}
	{
	// Counter*********************************************************************
		// 
		// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 2.0f, 8.0f); // doubled base plane size

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.5f, 1.0f, 6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.05, 0.14, 0.58, 1); // changed plane color to dark blueish

	SetShaderTexture("pbgradient");
	SetTextureUVScale(1.0f, 1.0f);

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
}
	{// Counter Top *********************************************************************
		// 
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(1.2f, 0.2f, 8.2f); // doubled base plane size

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-5.5f, 2.0f, 6.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.05, 0.14, 0.58, 1); // changed plane color to dark blueish

		SetShaderTexture("ciron");
		SetTextureUVScale(1.0f, 1.0f); 

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	{
		// Pillar*********************************************************************************
			// 
			// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(1.0f, 8.0f, 1.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-5.0f, 4.0f, 1.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.05, 0.14, 0.58, 1); // changed plane color to dark blueish

		SetShaderTexture("pbgradient");
		SetTextureUVScale(1.0f, 1.0f);

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	{
		// Pillar 2*********************************************************************************
			// 
			// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(1.0f, 8.0f, 1.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-5.0f, 4.0f, -3.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.05, 0.14, 0.58, 1); // changed plane color to dark blueish

		SetShaderTexture("pbgradient");
		SetTextureUVScale(1.0f, 1.0f);

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	{
		// Pillar 3*********************************************************************************
			// 
			// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(1.0f, 8.0f, 1.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-5.0f, 4.0f, 10.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.05, 0.14, 0.58, 1); // changed plane color to dark blueish

		SetShaderTexture("pbgradient");
		SetTextureUVScale(1.0f, 1.0f);

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	{
		// wall front left alley*********************************************************************************
			// 
			// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.5f, 24.0f, 10.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-10.0f, 12.0f, 15.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.05, 0.14, 0.58, 1); // changed plane color to dark blueish

		SetShaderTexture("bb");
		SetTextureUVScale(1.0f, 1.0f); // tiled texture using UV to increase the visual detail

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	{
		// wall front left alley accent*********************************************************************************
			// 
			// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.5f, 8.0f, 10.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-9.5f, 4.0f, 15.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.05, 0.14, 0.58, 1); // changed plane color to dark blueish

		SetShaderTexture("pbgradient");
		SetTextureUVScale(1.0f, 50.0f); // tiled texture using UV to increase the visual detail

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	{
		// ramen shop top accent*********************************************************************************
			// 
			// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.25f, 14.5f, 3.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 90.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-4.5f, 10.0f, 3.5f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.05, 0.14, 0.58, 1); // changed plane color to dark blueish

		SetShaderTexture("cshiplap");
		SetTextureUVScale(0.025f, 0.6f); // tiled texture using UV to increase the visual detail

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	{
		// ramen shop top accent 2*********************************************************************************
			// 
			// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.25f, 14.5f, 11.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 90.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-4.5f, 19.0f, 3.5f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.05, 0.14, 0.58, 1); // changed plane color to dark blueish

		SetShaderTexture("dbconcrete");
		SetTextureUVScale(0.025f, 0.6f); // tiled texture using UV to increase the visual detail

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	{
		// wall front left alley*********************************************************************************
			// 
			// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(10.0f, 24.0f, 0.5f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-15.25f, 12.0f, 19.75f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.05, 0.14, 0.58, 1); // changed plane color to dark blueish

		SetShaderTexture("bb");
		SetTextureUVScale(1.0f, 1.0f); 

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	{
		// wall ramen shop*********************************************************************************
			// 
			// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(10.0f, 8.0f, 0.5f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-10.0f, 4.0f, 10.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.05, 0.14, 0.58, 1); // changed plane color to dark blueish

		SetShaderTexture("bb");
		SetTextureUVScale(1.0f, 1.0f);

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	{
		// wall ramen shop*********************************************************************************
			// 
			// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(15.0f, 8.0f, 0.5f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-12.5f, 4.0f, -3.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.05, 0.14, 0.58, 1); // changed plane color to dark blueish

		SetShaderTexture("bb");
		SetTextureUVScale(1.0f, 1.0f);

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	{
		// wall ramen shop 2*********************************************************************************
			// 
			// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(15.0f, 16.0f, 14.5f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-12.0f, 16.25f, 3.5f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.05, 0.14, 0.58, 1); // changed plane color to dark blueish

		SetShaderTexture("bb");
		SetTextureUVScale(1.0f, 1.0f);

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	{
		// left building height partition *********************************************************************************
			// 
			// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(16.0f, 0.25f, 15.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-12.0f, 8.0f, 3.5f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.05, 0.14, 0.58, 1); // changed plane color to dark blueish

		SetShaderTexture("bb");
		SetTextureUVScale(1.0f, 1.0f);

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	{
		// left building height partition 2*********************************************************************************
			// 
			// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(16.0f, 0.25f, 15.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-12.0f, 8.27f, 3.5f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.05, 0.14, 0.58, 1); // changed plane color to dark blueish

		SetShaderTexture("bb");
		SetTextureUVScale(1.0f, 1.0f);

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
///////////////
	{
		// sidewalk *********************************************************************************
			// 
			// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(8.0f, 0.25f, 10.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-6.0f, 0.125f, 15.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.31, 0.66, 1.0, 1); // changed plane color to cyan

		//SetShaderTexture("lapis");
		//SetTextureUVScale(16.0f, 8.0f); 

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	{
		// sidewalk2 *********************************************************************************
			// 
			// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(8.0f, 0.1f, 10.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-5.0f, 0.05f, 15.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.31, 0.66, 1.0, 1); // changed plane color to cyan

		//SetShaderTexture("lapis");
		//SetTextureUVScale(16.0f, 8.0f); // tiled texture using UV to increase the visual detail

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	{
		// sidewalk right of front ramen *********************************************************************************
			// 
			// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(3.0f, 0.1f, 6.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-3.5f, 0.05f, -1.2f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.31, 0.66, 1.0, 1); // changed plane color to dark cyan

		//SetShaderTexture("lapis");
		//SetTextureUVScale(16.0f, 8.0f); // tiled texture using UV to increase the visual detail

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	{
		// sidewalk right of front ramen *********************************************************************************
			// 
			// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(2.0f, 0.1f, 4.5f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-6.0f, 0.05f, -0.5f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		//SetShaderColor(0.31, 0.66, 1.0, 1); // changed plane color to dark cyan

		SetShaderTexture("pbgradient");
		SetTextureUVScale(1.0f, 1.0f); // tiled texture using UV to increase the visual detail

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	{
		// straight ahead sidewalk *********************************************************************************
			// 
			// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(20.0f, 0.05f, 15.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(6.5f, 0.025f, -18.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.31, 0.66, 1.0, 1); // changed plane color to cyan

		//SetShaderTexture("lapis");
		//SetTextureUVScale(16.0f, 8.0f); // tiled texture using UV to increase the visual detail

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	{
		// sidewalk right side of alley *********************************************************************************
			// 
			// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(17.0f, 0.1f, 27.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(12.0f, 0.05f, 11.5f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.31, 0.66, 1.0, 1); // changed plane color to cyan

		//SetShaderTexture("lapis");
		//SetTextureUVScale(16.0f, 8.0f); // tiled texture using UV to increase the visual detail

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	{
		// sidewalk right side of alley building *********************************************************************************
			// 
			// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(14.5f, 24.0f, 24.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(13.25f, 12.0f, 10.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.31, 0.66, 1.0, 1); // changed plane color to cyan

		//SetShaderTexture("lapis");
		//SetTextureUVScale(16.0f, 8.0f); // tiled texture using UV to increase the visual detail

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	{
		// building right side foundation *********************************************************************************
			// 
			// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.01f, 2.5f, 24.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(6.0f, 1.25f, 10.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.01, 0.01, 0.28, 1); // changed plane color to blue

		//SetShaderTexture("lapis");
		//SetTextureUVScale(1.0f, 1.0f); // tiled texture using UV to increase the visual detail

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	{
		// building right side window 1 *********************************************************************************
			// 
		glm::vec3 basePos = glm::vec3(6.0f, 4.0f, 0.0f); // The original location
		glm::vec3 windowOffset = glm::vec3(0.0f, 0.0f, 0.0f);
			// set the XYZ scale for the mesh
		
		scaleXYZ = glm::vec3(0.02f, 8.0f, 2.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(6.0f, 4.0f, 0.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);


		SetShaderTexture("oygradient");
		SetTextureUVScale(1.0f, 1.0f); // tiled texture using UV to increase the visual detail

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
		
		// window frame
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.03f, 8.0f, 0.1f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(6.0f, 4.0f, -1.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);


		SetShaderTexture("pbgradient");
		SetTextureUVScale(1.0f, 1.0f); // tiled texture using UV to increase the visual detail


		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		// window frame
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.03f, 8.0f, 0.1f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(6.0f, 4.0f, 1.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);


		SetShaderTexture("pbgradient");
		SetTextureUVScale(1.0f, 1.0f); // tiled texture using UV to increase the visual detail

		
		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		// window frame
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.03f, 8.0f, 0.1f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(6.0f, 4.0f, 0.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);


		SetShaderTexture("pbgradient");
		SetTextureUVScale(1.0f, 1.0f); // tiled texture using UV to increase the visual detail

		
		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();

		// window frame
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.03f, 0.03f, 2.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(6.0f, 4.0f, 0.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);


		SetShaderTexture("pbgradient");
		SetTextureUVScale(1.0f, 1.0f); // tiled texture using UV to increase the visual detail

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
		// window frame
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.03f, 0.03f, 2.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(6.0f, 8.0f, 0.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);


		SetShaderTexture("pbgradient");
		SetTextureUVScale(1.0f, 1.0f); // tiled texture using UV to increase the visual detail

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
		// window frame
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.04f, 1.0f, 2.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(6.0f, 0.5f, 0.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);


		SetShaderTexture("pbgradient");
		SetTextureUVScale(1.0f, 1.0f); // tiled texture using UV to increase the visual detail

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	{// Window Template ********************************************************************
	// DEFINE THE OFFSET HERE
	glm::vec3 windowOffset = glm::vec3(0.0f, 0.0f, 4.0f); 
	glm::vec3 basePos = glm::vec3(6.0f, 4.0f, 0.0f);     

	// Helper to calculate position: (basePos + windowOffset)
	glm::vec3 currentPos = basePos + windowOffset;

	// --- WINDOW GLASS (The Pane) ---
	scaleXYZ = glm::vec3(0.02f, 8.0f, 2.0f);
	XrotationDegrees = 0.0f; YrotationDegrees = 0.0f; ZrotationDegrees = 0.0f;
	positionXYZ = currentPos; // Uses the offset position

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("pbgradient");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// --- WINDOW FRAME (Top/Bottom/Sides) ---

	// Frame Part 1: Top piece
	scaleXYZ = glm::vec3(0.03f, 8.0f, 0.1f);
	positionXYZ = currentPos + glm::vec3(0.0f, 0.0f, 0.0f); // Offset from center
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("pbgradient");
	m_basicMeshes->DrawBoxMesh();

	// Frame Part 2: Bottom piece
	scaleXYZ = glm::vec3(0.03f, 8.0f, 0.1f);
	positionXYZ = currentPos + glm::vec3(0.0f, 0.0f, 1.0f); // Offset from center
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("pbgradient");
	m_basicMeshes->DrawBoxMesh();

	// Frame Part 3: Vertical Center
	scaleXYZ = glm::vec3(0.04f, 0.03f, 2.0f);
	positionXYZ = currentPos;
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("pbgradient");
	m_basicMeshes->DrawBoxMesh();

	// Frame Part 4: Vertical Side (Left)
	scaleXYZ = glm::vec3(0.03f, 8.0f, 0.1f);
	positionXYZ = currentPos + glm::vec3(0.0f, 1.0f, -1.0f); // Moved up by 4 units
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("pbgradient");
	m_basicMeshes->DrawBoxMesh();

	// Frame Part 5: Vertical Side (Right)
	scaleXYZ = glm::vec3(0.03f, 0.03f, 2.0f);
	positionXYZ = currentPos - glm::vec3(0.0f, 4.0f, 0.0f); // Moved down by 4 units
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("pbgradient");
	m_basicMeshes->DrawBoxMesh();
	 
	// Frame Part 6: Bottom Horizontal Piece
	scaleXYZ = glm::vec3(0.04f, 1.0f, 2.0f);
	positionXYZ = currentPos - glm::vec3(0.0f, 3.5f, 0.0f); // Moved to the bottom edge
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("pbgradient");
	m_basicMeshes->DrawBoxMesh();

	// Frame Part 7: Middle Horizontal Piece
	scaleXYZ = glm::vec3(0.04f, 4.0f, 0.1f);
	positionXYZ = currentPos + glm::vec3(0.0f, 0.0f, 0.0f); // Offset from center
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("pbgradient");
	m_basicMeshes->DrawBoxMesh();

}//************************************************************************************
	{// Window Template ********************************************************************
	// DEFINE THE OFFSET HERE
		glm::vec3 windowOffset = glm::vec3(0.0f, 0.0f, 20.0f);
		glm::vec3 basePos = glm::vec3(6.0f, 4.0f, 0.0f);

		// Helper to calculate position: (basePos + windowOffset)
		glm::vec3 currentPos = basePos + windowOffset;

		// --- WINDOW GLASS (The Pane) ---
		scaleXYZ = glm::vec3(0.02f, 8.0f, 2.0f);
		XrotationDegrees = 0.0f; YrotationDegrees = 0.0f; ZrotationDegrees = 0.0f;
		positionXYZ = currentPos; // Uses the offset position

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderTexture("pbgradient");
		SetTextureUVScale(1.0f, 1.0f);
		m_basicMeshes->DrawBoxMesh();

		// --- WINDOW FRAME (Top/Bottom/Sides) ---

		// Frame Part 1: Top piece
		scaleXYZ = glm::vec3(0.03f, 8.0f, 0.1f);
		positionXYZ = currentPos + glm::vec3(0.0f, 0.0f, 0.0f); // Offset from center
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderTexture("pbgradient");
		m_basicMeshes->DrawBoxMesh();

		// Frame Part 2: Bottom piece
		scaleXYZ = glm::vec3(0.03f, 8.0f, 0.1f);
		positionXYZ = currentPos + glm::vec3(0.0f, 0.0f, 1.0f); // Offset from center
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderTexture("pbgradient");
		m_basicMeshes->DrawBoxMesh();

		// Frame Part 3: Vertical Center
		scaleXYZ = glm::vec3(0.04f, 0.03f, 2.0f);
		positionXYZ = currentPos;
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderTexture("pbgradient");
		m_basicMeshes->DrawBoxMesh();

		// Frame Part 4: Vertical Side (Left)
		scaleXYZ = glm::vec3(0.03f, 8.0f, 0.1f);
		positionXYZ = currentPos + glm::vec3(0.0f, 1.0f, -1.0f); // Moved up by 4 units
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderTexture("pbgradient");
		m_basicMeshes->DrawBoxMesh();

		// Frame Part 5: Vertical Side (Right)
		scaleXYZ = glm::vec3(0.03f, 0.03f, 2.0f);
		positionXYZ = currentPos - glm::vec3(0.0f, 4.0f, 0.0f); // Moved down by 4 units
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderTexture("pbgradient");
		m_basicMeshes->DrawBoxMesh();

		// Frame Part 6: Bottom Horizontal Piece
		scaleXYZ = glm::vec3(0.04f, 1.0f, 2.0f);
		positionXYZ = currentPos - glm::vec3(0.0f, 3.5f, 0.0f); // Moved to the bottom edge
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderTexture("pbgradient");
		m_basicMeshes->DrawBoxMesh();

		// Frame Part 7: Middle Horizontal Piece
		scaleXYZ = glm::vec3(0.04f, 4.0f, 0.1f);
		positionXYZ = currentPos + glm::vec3(0.0f, 0.0f, 0.0f); // Offset from center
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderTexture("pbgradient");
		m_basicMeshes->DrawBoxMesh();

	}//************************************************************************************
	{// Window Template ********************************************************************
	// DEFINE THE OFFSET HERE
		glm::vec3 windowOffset = glm::vec3(0.0f, 0.0f, 8.0f);
		glm::vec3 basePos = glm::vec3(6.0f, 4.0f, 0.0f);

		// Helper to calculate position: (basePos + windowOffset)
		glm::vec3 currentPos = basePos + windowOffset;

		// --- WINDOW GLASS (The Pane) ---
		scaleXYZ = glm::vec3(0.02f, 8.0f, 2.0f);
		XrotationDegrees = 0.0f; YrotationDegrees = 0.0f; ZrotationDegrees = 0.0f;
		positionXYZ = currentPos; // Uses the offset position

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderTexture("oygradient");
		SetTextureUVScale(1.0f, 1.0f);
		m_basicMeshes->DrawBoxMesh();

		// --- WINDOW FRAME (Top/Bottom/Sides) ---

		// Frame Part 1: Top piece
		scaleXYZ = glm::vec3(0.03f, 8.0f, 0.1f);
		positionXYZ = currentPos + glm::vec3(0.0f, 0.0f, 0.0f); // Offset from center
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderTexture("pbgradient");
		m_basicMeshes->DrawBoxMesh();

		// Frame Part 2: Bottom piece
		scaleXYZ = glm::vec3(0.03f, 8.0f, 0.1f);
		positionXYZ = currentPos + glm::vec3(0.0f, 0.0f, 1.0f); // Offset from center
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderTexture("pbgradient");
		m_basicMeshes->DrawBoxMesh();

		// Frame Part 3: Vertical Center
		scaleXYZ = glm::vec3(0.04f, 0.03f, 2.0f);
		positionXYZ = currentPos;
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderTexture("pbgradient");
		m_basicMeshes->DrawBoxMesh();

		// Frame Part 4: Vertical Side (Left)
		scaleXYZ = glm::vec3(0.03f, 8.0f, 0.1f);
		positionXYZ = currentPos + glm::vec3(0.0f, 1.0f, -1.0f); // Moved up by 4 units
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderTexture("pbgradient");
		m_basicMeshes->DrawBoxMesh();

		// Frame Part 5: Vertical Side (Right)
		scaleXYZ = glm::vec3(0.03f, 0.03f, 2.0f);
		positionXYZ = currentPos - glm::vec3(0.0f, 4.0f, 0.0f); // Moved down by 4 units
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderTexture("pbgradient");
		m_basicMeshes->DrawBoxMesh();

		// Frame Part 6: Bottom Horizontal Piece
		scaleXYZ = glm::vec3(0.04f, 1.0f, 2.0f);
		positionXYZ = currentPos - glm::vec3(0.0f, 3.5f, 0.0f); // Moved to the bottom edge
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderTexture("pbgradient");
		m_basicMeshes->DrawBoxMesh();

		// Frame Part 7: Middle Horizontal Piece
		scaleXYZ = glm::vec3(0.04f, 4.0f, 0.1f);
		positionXYZ = currentPos + glm::vec3(0.0f, 0.0f, 0.0f); // Offset from center
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderTexture("pbgradient");
		m_basicMeshes->DrawBoxMesh();

	}//************************************************************************************
	{// Window Template ********************************************************************
	// DEFINE THE OFFSET HERE
		glm::vec3 windowOffset = glm::vec3(0.0f, 0.0f, 12.0f);
		glm::vec3 basePos = glm::vec3(6.0f, 4.0f, 0.0f);

		// Helper to calculate position: (basePos + windowOffset)
		glm::vec3 currentPos = basePos + windowOffset;

		// --- WINDOW GLASS (The Pane) ---
		scaleXYZ = glm::vec3(0.02f, 8.0f, 2.0f);
		XrotationDegrees = 0.0f; YrotationDegrees = 0.0f; ZrotationDegrees = 0.0f;
		positionXYZ = currentPos; // Uses the offset position

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderTexture("pbgradient");
		SetTextureUVScale(1.0f, 1.0f);
		m_basicMeshes->DrawBoxMesh();

		// --- WINDOW FRAME (Top/Bottom/Sides) ---

		// Frame Part 1: Top piece
		scaleXYZ = glm::vec3(0.03f, 8.0f, 0.1f);
		positionXYZ = currentPos + glm::vec3(0.0f, 0.0f, 0.0f); // Offset from center
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderTexture("pbgradient");
		m_basicMeshes->DrawBoxMesh();

		// Frame Part 2: Bottom piece
		scaleXYZ = glm::vec3(0.03f, 8.0f, 0.1f);
		positionXYZ = currentPos + glm::vec3(0.0f, 0.0f, 1.0f); // Offset from center
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderTexture("pbgradient");
		m_basicMeshes->DrawBoxMesh();

		// Frame Part 3: Vertical Center
		scaleXYZ = glm::vec3(0.04f, 0.03f, 2.0f);
		positionXYZ = currentPos;
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderTexture("pbgradient");
		m_basicMeshes->DrawBoxMesh();

		// Frame Part 4: Vertical Side (Left)
		scaleXYZ = glm::vec3(0.03f, 8.0f, 0.1f);
		positionXYZ = currentPos + glm::vec3(0.0f, 1.0f, -1.0f); // Moved up by 4 units
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderTexture("pbgradient");
		m_basicMeshes->DrawBoxMesh();

		// Frame Part 5: Vertical Side (Right)
		scaleXYZ = glm::vec3(0.03f, 0.03f, 2.0f);
		positionXYZ = currentPos - glm::vec3(0.0f, 4.0f, 0.0f); // Moved down by 4 units
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderTexture("pbgradient");
		m_basicMeshes->DrawBoxMesh();

		// Frame Part 6: Bottom Horizontal Piece
		scaleXYZ = glm::vec3(0.04f, 1.0f, 2.0f);
		positionXYZ = currentPos - glm::vec3(0.0f, 3.5f, 0.0f); // Moved to the bottom edge
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderTexture("pbgradient");
		m_basicMeshes->DrawBoxMesh();

		// Frame Part 7: Middle Horizontal Piece
		scaleXYZ = glm::vec3(0.04f, 4.0f, 0.1f);
		positionXYZ = currentPos + glm::vec3(0.0f, 0.0f, 0.0f); // Offset from center
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderTexture("pbgradient");
		m_basicMeshes->DrawBoxMesh();

	}//************************************************************************************
	{// Window Template ********************************************************************
	// DEFINE THE OFFSET HERE
		glm::vec3 windowOffset = glm::vec3(0.0f, 0.0f, 16.0f);
		glm::vec3 basePos = glm::vec3(6.0f, 4.0f, 0.0f);

		// Helper to calculate position: (basePos + windowOffset)
		glm::vec3 currentPos = basePos + windowOffset;

		// --- WINDOW GLASS (The Pane) ---
		scaleXYZ = glm::vec3(0.02f, 8.0f, 2.0f);
		XrotationDegrees = 0.0f; YrotationDegrees = 0.0f; ZrotationDegrees = 0.0f;
		positionXYZ = currentPos; // Uses the offset position

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderTexture("oygradient");
		SetTextureUVScale(1.0f, 1.0f);
		m_basicMeshes->DrawBoxMesh();

		// --- WINDOW FRAME (Top/Bottom/Sides) ---

		// Frame Part 1: Top piece
		scaleXYZ = glm::vec3(0.03f, 8.0f, 0.1f);
		positionXYZ = currentPos + glm::vec3(0.0f, 0.0f, 0.0f); // Offset from center
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderTexture("pbgradient");
		m_basicMeshes->DrawBoxMesh();

		// Frame Part 2: Bottom piece
		scaleXYZ = glm::vec3(0.03f, 8.0f, 0.1f);
		positionXYZ = currentPos + glm::vec3(0.0f, 0.0f, 1.0f); // Offset from center
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderTexture("pbgradient");
		m_basicMeshes->DrawBoxMesh();

		// Frame Part 3: Vertical Center
		scaleXYZ = glm::vec3(0.04f, 0.03f, 2.0f);
		positionXYZ = currentPos;
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderTexture("pbgradient");
		m_basicMeshes->DrawBoxMesh();

		// Frame Part 4: Vertical Side (Left)
		scaleXYZ = glm::vec3(0.03f, 8.0f, 0.1f);
		positionXYZ = currentPos + glm::vec3(0.0f, 1.0f, -1.0f); // Moved up by 4 units
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderTexture("pbgradient");
		m_basicMeshes->DrawBoxMesh();

		// Frame Part 5: Vertical Side (Right)
		scaleXYZ = glm::vec3(0.03f, 0.03f, 2.0f);
		positionXYZ = currentPos - glm::vec3(0.0f, 4.0f, 0.0f); // Moved down by 4 units
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderTexture("pbgradient");
		m_basicMeshes->DrawBoxMesh();

		// Frame Part 6: Bottom Horizontal Piece
		scaleXYZ = glm::vec3(0.04f, 1.0f, 2.0f);
		positionXYZ = currentPos - glm::vec3(0.0f, 3.5f, 0.0f); // Moved to the bottom edge
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderTexture("pbgradient");
		m_basicMeshes->DrawBoxMesh();

		// Frame Part 7: Middle Horizontal Piece
		scaleXYZ = glm::vec3(0.04f, 4.0f, 0.1f);
		positionXYZ = currentPos + glm::vec3(0.0f, 0.0f, 0.0f); // Offset from center
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderTexture("pbgradient");
		m_basicMeshes->DrawBoxMesh();

	}//************************************************************************************
	{
		// sidewalk right side of alley building overhead *********************************************************************************
			// 
			// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(1.0f, 16.0f, 24.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(5.5f, 16.0f, 10.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.31, 0.66, 1.0, 1); 

		SetShaderTexture("dbconcrete");
		SetTextureUVScale(1.0f, 1.0f); 

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	{
		// sidewalk right side of alley building overhead *********************************************************************************
			// 
			// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.5f, 16.0f, 24.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(4.75f, 16.0f, 10.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.31, 0.66, 1.0, 1);

		SetShaderTexture("bb");
		SetTextureUVScale(1.0f, 100.0f);

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}

// Yakitori blocks
	{ //blocked stool to isolate from other stool copies 
	//
	// STOOL 1 CENTER
	//
		glm::vec3 stoolOffset1 = glm::vec3(1.0f, 0.0f, -12.0f); // used to control offset of stool from origin position

		// Seat
		scaleXYZ = glm::vec3(0.55f, 0.20f, 0.55f);
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;
		positionXYZ = stoolOffset1 + glm::vec3(0.0f, 1.55f, 0.0f);

		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		// SetShaderColor(0.90f, 0.25f, 0.75f, 0.9f);
		// Color changed to texture
		
		{ // top of seat
			SetShaderTexture("pconcrete");
			SetTextureUVScale(1.0f, 1.0f);
			m_basicMeshes->DrawCylinderMesh(true, false, false);
		}
		{ // bottom of seat
			SetShaderTexture("pconcrete");
			SetTextureUVScale(0.25f, 0.25f);
			m_basicMeshes->DrawCylinderMesh(false, true, false);
		}
		{ // side of seat
			SetShaderTexture("pconcrete");
			SetTextureUVScale(1.0f, 0.25f);
			m_basicMeshes->DrawCylinderMesh(false, false, true);
		}

		// Legs
			// blanket variables for the all stool 1 legs; scale, position, and angle
		glm::vec3 legScale = glm::vec3(0.03f, 1.55f, 0.03f);
		float legY = 0.01f;
		float legAngle = 6.0f;

		// Front-right leg
		SetTransformations(
			legScale,
			-legAngle,
			0.0f,
			legAngle,
			stoolOffset1 + glm::vec3(0.44f, legY, 0.44f));
		//SetShaderColor(0.90f, 0.25f, 0.75f, 1.0f); // neopink
		SetShaderTexture("pmetal");
		SetTextureUVScale(0.5f, 0.5f);

		m_basicMeshes->DrawCylinderMesh();

		// Front-left leg
		SetTransformations(
			legScale,
			-legAngle,
			0.0f,
			-legAngle,
			stoolOffset1 + glm::vec3(-0.44f, legY, 0.44f));
		//SetShaderColor(0.90f, 0.25f, 0.75f, 1.0f); // neopink
		SetShaderTexture("pmetal");

		m_basicMeshes->DrawCylinderMesh();

		// Back-right leg
		SetTransformations(
			legScale,
			legAngle,
			0.0f,
			legAngle,
			stoolOffset1 + glm::vec3(0.44f, legY, -0.44f));
		//SetShaderColor(0.90f, 0.25f, 0.75f, 1.0f); // neopink
		SetShaderTexture("pmetal");

		m_basicMeshes->DrawCylinderMesh();

		// Back-left leg
		SetTransformations(
			legScale,
			legAngle,
			0.0f,
			-legAngle,
			stoolOffset1 + glm::vec3(-0.44f, legY, -0.44f));
		//SetShaderColor(0.90f, 0.25f, 0.75f, 1.0f); // neopink
		SetShaderTexture("pmetal");

		m_basicMeshes->DrawCylinderMesh();


		// Footrest made from cylinders
		//SetShaderColor(0.90f, 0.25f, 0.75f, 1.0f); // neopink
		SetShaderTexture("pmetal");



		// blanket variables for all stool 1 foot bars; spread, height, radius, and length
		float legSpread = 0.44f;
		float footBarY = 0.50f;
		float footBarRadius = 0.03f;
		float footBarLength = legSpread * 2.0f;

		// front bar: starts at right front leg, extends to left
		SetTransformations(
			glm::vec3(footBarRadius, footBarLength, footBarRadius),
			0.0f,
			0.0f,
			90.0f,
			stoolOffset1 + glm::vec3(legSpread, footBarY, legSpread));
		m_basicMeshes->DrawCylinderMesh();

		// back bar: starts at right back leg, extends to left
		SetTransformations(
			glm::vec3(footBarRadius, footBarLength, footBarRadius),
			0.0f,
			0.0f,
			90.0f,
			stoolOffset1 + glm::vec3(legSpread, footBarY, -legSpread));
		m_basicMeshes->DrawCylinderMesh();

		// right bar: starts at back right leg, extends to front
		SetTransformations(
			glm::vec3(footBarRadius, footBarLength, footBarRadius),
			90.0f,
			0.0f,
			0.0f,
			stoolOffset1 + glm::vec3(legSpread, footBarY, -legSpread));
		m_basicMeshes->DrawCylinderMesh();

		// left bar: starts at back left leg, extends to front
		SetTransformations(
			glm::vec3(footBarRadius, footBarLength, footBarRadius),
			90.0f,
			0.0f,
			0.0f,
			stoolOffset1 + glm::vec3(-legSpread, footBarY, -legSpread));
		m_basicMeshes->DrawCylinderMesh();
	}


	{
		//
		// STOOL 2 Front Left DUPLICATE OF STOOL 1
		//
		glm::vec3 stoolOffset2 = glm::vec3(2.5f, 0.0f, -12.0f);

		// Seat

		scaleXYZ = glm::vec3(0.55f, 0.20f, 0.55f);
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;
		positionXYZ = stoolOffset2 + glm::vec3(0.0f, 1.55f, 0.0f);

		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		// HOLD SetShaderColor(0.90f, 0.25f, 0.75f, 0.9f);
		SetShaderTexture("pconcrete");
		SetTextureUVScale(1.0, 1.0);
		m_basicMeshes->DrawCylinderMesh();


		// Legs
		glm::vec3 legScale = glm::vec3(0.03f, 1.55f, 0.03f);
		float legY = 0.01f;
		float legAngle = 6.0f;
		SetShaderTexture("pmetal");
		SetTextureUVScale(0.5f, 0.5f);

		// Front-right leg
		SetTransformations(
			legScale,
			-legAngle,
			0.0f,
			legAngle,
			stoolOffset2 + glm::vec3(0.44f, legY, 0.44f));
		//HOLD SetShaderColor(0.90f, 0.25f, 0.75f, 1.0f);
		m_basicMeshes->DrawCylinderMesh();

		// Front-left leg
		SetTransformations(
			legScale,
			-legAngle,
			0.0f,
			-legAngle,
			stoolOffset2 + glm::vec3(-0.44f, legY, 0.44f));
		//HOLD SetShaderColor(0.90f, 0.25f, 0.75f, 1.0f);
		m_basicMeshes->DrawCylinderMesh();

		// Back-right leg
		SetTransformations(
			legScale,
			legAngle,
			0.0f,
			legAngle,
			stoolOffset2 + glm::vec3(0.44f, legY, -0.44f));
		//HOLD SetShaderColor(0.90f, 0.25f, 0.75f, 1.0f);
		m_basicMeshes->DrawCylinderMesh();

		// Back-left leg
		SetTransformations(
			legScale,
			legAngle,
			0.0f,
			-legAngle,
			stoolOffset2 + glm::vec3(-0.44f, legY, -0.44f));
		//HOLD SetShaderColor(0.90f, 0.25f, 0.75f, 1.0f);
		m_basicMeshes->DrawCylinderMesh();


		// Footrest made from cylinders
		//HOLD SetShaderColor(0.90f, 0.25f, 0.75f, 1.0f);
		SetShaderTexture("pmetal");

		float legSpread = 0.44f;
		float footBarY = 0.50f;
		float footBarRadius = 0.03f;
		float footBarLength = legSpread * 2.0f;

		// front bar: starts at right front leg, extends to left
		SetTransformations(
			glm::vec3(footBarRadius, footBarLength, footBarRadius),
			0.0f,
			0.0f,
			90.0f,
			stoolOffset2 + glm::vec3(legSpread, footBarY, legSpread));
		m_basicMeshes->DrawCylinderMesh();

		// back bar: starts at right back leg, extends to left
		SetTransformations(
			glm::vec3(footBarRadius, footBarLength, footBarRadius),
			0.0f,
			0.0f,
			90.0f,
			stoolOffset2 + glm::vec3(legSpread, footBarY, -legSpread));
		m_basicMeshes->DrawCylinderMesh();

		// right bar: starts at back right leg, extends to front
		SetTransformations(
			glm::vec3(footBarRadius, footBarLength, footBarRadius),
			90.0f,
			0.0f,
			0.0f,
			stoolOffset2 + glm::vec3(legSpread, footBarY, -legSpread));
		m_basicMeshes->DrawCylinderMesh();

		// left bar: starts at back left leg, extends to front
		SetTransformations(
			glm::vec3(footBarRadius, footBarLength, footBarRadius),
			90.0f,
			0.0f,
			0.0f,
			stoolOffset2 + glm::vec3(-legSpread, footBarY, -legSpread));
		m_basicMeshes->DrawCylinderMesh();
	}

	{
		//
		// STOOL 3 Front Left DUPLICATE OF STOOL 1
		//
		glm::vec3 stoolOffset3 = glm::vec3(4.0f, 0.0f, -12.0f);

		// Seat

		scaleXYZ = glm::vec3(0.55f, 0.20f, 0.55f);
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;
		positionXYZ = stoolOffset3 + glm::vec3(0.0f, 1.55f, 0.0f);

		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		//HOLD SetShaderColor(0.90f, 0.25f, 0.75f, 0.9f);
		SetShaderTexture("pconcrete");
		SetTextureUVScale(1.0f, 1.0f);

		m_basicMeshes->DrawCylinderMesh();



		// Legs
		glm::vec3 legScale = glm::vec3(0.03f, 1.55f, 0.03f);
		float legY = 0.01f;
		float legAngle = 6.0f;
		SetShaderTexture("pmetal");

		// Front-right leg
		SetTransformations(
			legScale,
			-legAngle,
			0.0f,
			legAngle,
			stoolOffset3 + glm::vec3(0.44f, legY, 0.44f));
		//HOLD SetShaderColor(0.90f, 0.25f, 0.75f, 1.0f);
		m_basicMeshes->DrawCylinderMesh();

		// Front-left leg
		SetTransformations(
			legScale,
			-legAngle,
			0.0f,
			-legAngle,
			stoolOffset3 + glm::vec3(-0.44f, legY, 0.44f));
		//HOLD SetShaderColor(0.90f, 0.25f, 0.75f, 1.0f);
		m_basicMeshes->DrawCylinderMesh();

		// Back-right leg
		SetTransformations(
			legScale,
			legAngle,
			0.0f,
			legAngle,
			stoolOffset3 + glm::vec3(0.44f, legY, -0.44f));
		//HOLD SetShaderColor(0.90f, 0.25f, 0.75f, 1.0f);
		m_basicMeshes->DrawCylinderMesh();

		// Back-left leg
		SetTransformations(
			legScale,
			legAngle,
			0.0f,
			-legAngle,
			stoolOffset3 + glm::vec3(-0.44f, legY, -0.44f));
		//HOLD SetShaderColor(0.90f, 0.25f, 0.75f, 1.0f);
		m_basicMeshes->DrawCylinderMesh();


		// Footrest made from cylinders
		//HOLD SetShaderColor(0.90f, 0.25f, 0.75f, 1.0f);
		SetShaderTexture("pmetal");

		float legSpread = 0.44f;
		float footBarY = 0.50f;
		float footBarRadius = 0.03f;
		float footBarLength = legSpread * 2.0f;

		// front bar: starts at right front leg, extends to left
		SetTransformations(
			glm::vec3(footBarRadius, footBarLength, footBarRadius),
			0.0f,
			0.0f,
			90.0f,
			stoolOffset3 + glm::vec3(legSpread, footBarY, legSpread));
		m_basicMeshes->DrawCylinderMesh();

		// back bar: starts at right back leg, extends to left
		SetTransformations(
			glm::vec3(footBarRadius, footBarLength, footBarRadius),
			0.0f,
			0.0f,
			90.0f,
			stoolOffset3 + glm::vec3(legSpread, footBarY, -legSpread));
		m_basicMeshes->DrawCylinderMesh();

		// right bar: starts at back right leg, extends to front
		SetTransformations(
			glm::vec3(footBarRadius, footBarLength, footBarRadius),
			90.0f,
			0.0f,
			0.0f,
			stoolOffset3 + glm::vec3(legSpread, footBarY, -legSpread));
		m_basicMeshes->DrawCylinderMesh();

		// left bar: starts at back left leg, extends to front
		SetTransformations(
			glm::vec3(footBarRadius, footBarLength, footBarRadius),
			90.0f,
			0.0f,
			0.0f,
			stoolOffset3 + glm::vec3(-legSpread, footBarY, -legSpread));
		m_basicMeshes->DrawCylinderMesh();
	}
	{
		//
		// STOOL 4 Front Left DUPLICATE OF STOOL 1
		//
		glm::vec3 stoolOffset3 = glm::vec3(-0.75f, 0.0f, -13.5f);

		// Seat

		scaleXYZ = glm::vec3(0.55f, 0.20f, 0.55f);
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;
		positionXYZ = stoolOffset3 + glm::vec3(0.0f, 1.55f, 0.0f);

		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		//HOLD SetShaderColor(0.90f, 0.25f, 0.75f, 0.9f);
		SetShaderTexture("pconcrete");
		SetTextureUVScale(1.0f, 1.0f);

		m_basicMeshes->DrawCylinderMesh();



		// Legs
		glm::vec3 legScale = glm::vec3(0.03f, 1.55f, 0.03f);
		float legY = 0.01f;
		float legAngle = 6.0f;
		SetShaderTexture("pmetal");

		// Front-right leg
		SetTransformations(
			legScale,
			-legAngle,
			0.0f,
			legAngle,
			stoolOffset3 + glm::vec3(0.44f, legY, 0.44f));
		//HOLD SetShaderColor(0.90f, 0.25f, 0.75f, 1.0f);
		m_basicMeshes->DrawCylinderMesh();

		// Front-left leg
		SetTransformations(
			legScale,
			-legAngle,
			0.0f,
			-legAngle,
			stoolOffset3 + glm::vec3(-0.44f, legY, 0.44f));
		//HOLD SetShaderColor(0.90f, 0.25f, 0.75f, 1.0f);
		m_basicMeshes->DrawCylinderMesh();

		// Back-right leg
		SetTransformations(
			legScale,
			legAngle,
			0.0f,
			legAngle,
			stoolOffset3 + glm::vec3(0.44f, legY, -0.44f));
		//HOLD SetShaderColor(0.90f, 0.25f, 0.75f, 1.0f);
		m_basicMeshes->DrawCylinderMesh();

		// Back-left leg
		SetTransformations(
			legScale,
			legAngle,
			0.0f,
			-legAngle,
			stoolOffset3 + glm::vec3(-0.44f, legY, -0.44f));
		//HOLD SetShaderColor(0.90f, 0.25f, 0.75f, 1.0f);
		m_basicMeshes->DrawCylinderMesh();


		// Footrest made from cylinders
		//HOLD SetShaderColor(0.90f, 0.25f, 0.75f, 1.0f);
		SetShaderTexture("pmetal");

		float legSpread = 0.44f;
		float footBarY = 0.50f;
		float footBarRadius = 0.03f;
		float footBarLength = legSpread * 2.0f;

		// front bar: starts at right front leg, extends to left
		SetTransformations(
			glm::vec3(footBarRadius, footBarLength, footBarRadius),
			0.0f,
			0.0f,
			90.0f,
			stoolOffset3 + glm::vec3(legSpread, footBarY, legSpread));
		m_basicMeshes->DrawCylinderMesh();

		// back bar: starts at right back leg, extends to left
		SetTransformations(
			glm::vec3(footBarRadius, footBarLength, footBarRadius),
			0.0f,
			0.0f,
			90.0f,
			stoolOffset3 + glm::vec3(legSpread, footBarY, -legSpread));
		m_basicMeshes->DrawCylinderMesh();

		// right bar: starts at back right leg, extends to front
		SetTransformations(
			glm::vec3(footBarRadius, footBarLength, footBarRadius),
			90.0f,
			0.0f,
			0.0f,
			stoolOffset3 + glm::vec3(legSpread, footBarY, -legSpread));
		m_basicMeshes->DrawCylinderMesh();

		// left bar: starts at back left leg, extends to front
		SetTransformations(
			glm::vec3(footBarRadius, footBarLength, footBarRadius),
			90.0f,
			0.0f,
			0.0f,
			stoolOffset3 + glm::vec3(-legSpread, footBarY, -legSpread));
		m_basicMeshes->DrawCylinderMesh();
	}
	{
		// Counter*********************************************************************
			// 
			// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(3.0f, 6.0f, 1.0f); 

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 90.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(4.5f, 1.5f, -14.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.05, 0.14, 0.58, 1); // changed plane color to dark blueish

		SetShaderTexture("cshiplap");
		SetTextureUVScale(1.0f, 1.5f); 

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	{// Counter Top *********************************************************************
		// 
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(6.5f, 0.1f, 1.2f); 

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(4.5f, 3.0f, -14.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		//SetShaderColor(0.05, 0.14, 0.58, 1); // changed plane color to dark blueish

		SetShaderTexture("ciron");
		SetTextureUVScale(1.0f, 1.0f); // tiled texture using UV to increase the visual detail

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	{// Counter Top Grill*********************************************************************
		// 
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(3.0f, 1.0f, 1.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(4.5f, 3.5f, -14.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		//SetShaderColor(0.05, 0.14, 0.58, 1); // changed plane color to dark blueish

		SetShaderTexture("ciron");
		SetTextureUVScale(1.0f, 1.0f); // tiled texture using UV to increase the visual detail

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	{// Pan 1 *********************************************************************
		// 
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.4f, 0.1f, 0.4f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 90.0f;
		ZrotationDegrees = 90.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(2.5f, 2.2f, -13.5f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		//SetShaderColor(0.05, 0.14, 0.58, 1); // changed plane color to dark blueish

		SetShaderTexture("ciron");
		SetTextureUVScale(1.0f, 1.0f); // tiled texture using UV to increase the visual detail

		// draw the mesh with transformation values
		m_basicMeshes->DrawCylinderMesh();

		// HANDLE 
		// 1. Scale: Keep it long and thin
		scaleXYZ = glm::vec3(0.5f, 0.1f, 0.1f); // Made it much larger to be easier to find

		// 2. Rotation: Identical to pan
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 45.0f;

		// 3. Position
		positionXYZ = glm::vec3(2.95f, 2.65f, -13.5f);

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		SetShaderTexture("ciron");

		m_basicMeshes->DrawBoxMesh();
	}
	{// Pan 2 *********************************************************************
		// 
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.5f, 0.1f, 0.5f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 90.0f;
		ZrotationDegrees = 90.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(4.0f, 2.0f, -13.5f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		//SetShaderColor(0.05, 0.14, 0.58, 1); // changed plane color to dark blueish

		SetShaderTexture("ciron");
		SetTextureUVScale(1.0f, 1.0f); // tiled texture using UV to increase the visual detail

		// draw the mesh with transformation values
		m_basicMeshes->DrawCylinderMesh();


		// HANDLE
		// 1. Scale: Keep it long and thin
		scaleXYZ = glm::vec3(0.5f, 0.1f, 0.1f); 

		// 2. Rotation: Identical to pan
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 45.0f;

		// 3. Position
		positionXYZ = glm::vec3(4.5f, 2.5f, -13.5f);

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		SetShaderTexture("ciron");

		m_basicMeshes->DrawBoxMesh();
	}
	{
		// yakitori shop left wall *********************************************************************************
			// 
			// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.5f, 8.0f, 12.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(0.0f, 4.0f, -19.5f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.31, 0.66, 1.0, 1); // changed plane color to dark blueish

		SetShaderTexture("bb");
		SetTextureUVScale(1.0f, 1.0f);

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	{
		// yakitori left pillar *********************************************************************************
			// 
			// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.5f, 8.0f, 0.5f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(0.0f, 4.0f, -13.5f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.05, 0.14, 0.58, 1); // changed plane color to dark blueish

		SetShaderTexture("pbgradient");
		SetTextureUVScale(1.0f, 1.0f);

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	{
		// yakitori shop right wall *********************************************************************************
			// 
			// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.5f, 8.0f, 12.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(12.0f, 4.0f, -19.5f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.05, 0.14, 0.58, 1); // changed plane color to dark blueish

		SetShaderTexture("dbconcrete");
		SetTextureUVScale(1.0f, 1.0f);

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	{
		// yakitori right pillar *********************************************************************************
			// 
			// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.5f, 8.0f, 0.5f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(12.0f, 4.0f, -13.5f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.05, 0.14, 0.58, 1); // changed plane color to dark blueish

		SetShaderTexture("pbgradient");
		SetTextureUVScale(1.0f, 1.0f);

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	{
		// yakitori shop back wall *********************************************************************************
			// 
			// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(12.0f, 8.0f, 0.5f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(6.0f, 4.0f, -25.5f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.05, 0.14, 0.58, 1); // changed plane color to dark blueish

		SetShaderTexture("dbconcrete");
		SetTextureUVScale(1.0f, 1.0f);

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	{
		// yakitori roof *********************************************************************************
			// 
			// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(4.0f, 13.5f, 12.5f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 90.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(5.5f, 10.0f, -19.5f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);


		SetShaderTexture("dbconcrete");
		SetTextureUVScale(0.5f, 8.0f);

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	{
		// yakitori roof 2*********************************************************************************
			// 
			// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(8.0f, 13.5f, 12.5f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 90.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(5.5f, 16.0f, -19.5f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);


		SetShaderTexture("cshiplap");
		SetTextureUVScale(1.0f, 2.0f);

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	{
		// yakitori roof 3*********************************************************************************
			// 
			// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(14.0f, 0.25f, 12.5f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(5.5f, 16.0f, -19.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);


		SetShaderTexture("cshiplap");
		SetTextureUVScale(1.0f, 2.0f);

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	{
		// yakitori roof overhang *********************************************************************************
			// 
			// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(3.5f, 0.125f, 13.5f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 90.0f;
		ZrotationDegrees = 30.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(5.5f, 8.0f, -12.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderTexture("cshiplap");
		SetTextureUVScale(1.0f, 10.0f); // tiled texture using UV to increase the visual detail

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	{
		// yakitori shop left wall outer*********************************************************************************
			// 
			// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(1.0f, 8.8f, 10.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-0.5f, 4.4f, -20.5f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.31, 0.66, 1.0, 1); // changed plane color to dark blueish

		SetShaderTexture("cshiplap");
		SetTextureUVScale(1.0f, 1.0f); // tiled texture using UV to increase the visual detail

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	{
		// yakitori left roof post *********************************************************************************
			// 
			// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.25f, 8.0f, 0.25f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-1.0f, 4.0f, -12.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.05, 0.14, 0.58, 1); // changed plane color to dark blueish

		SetShaderTexture("pbgradient");
		SetTextureUVScale(1.0f, 1.0f); 

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	{
		// yakitori right roof post *********************************************************************************
			// 
			// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.25f, 8.0f, 0.25f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(12.0f, 4.0f, -12.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.05, 0.14, 0.58, 1); // changed plane color to dark blueish

		SetShaderTexture("pbgradient");
		SetTextureUVScale(1.0f, 1.0f);

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	{
		// utility post *********************************************************************************
			// 
			// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.125f, 6.0f, 0.125f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(5.5f, 0.0f, -3.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.05, 0.14, 0.58, 1); // changed plane color to dark blueish

		SetShaderTexture("pbgradient");
		SetTextureUVScale(1.0f, 1.0f);

		// draw the mesh with transformation values
		m_basicMeshes->DrawCylinderMesh();
	}
	{
		// utility post 2*********************************************************************************
			// 
			// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.125f, 6.0f, 0.125f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(5.5f, 6.0f, -3.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.31, 0.66, 1.0, 1); // changed plane color to dark blueish

		SetShaderTexture("dbconcrete");
		SetTextureUVScale(1.0f, 1.0f);

		// draw the mesh with transformation values
		m_basicMeshes->DrawCylinderMesh();
	}
	{// Assets 
		// noren / nobori *********************************************************************************
			// 
		glm::vec3 basePos = glm::vec3(6.0f, 4.0f, 0.0f); // The original location
		glm::vec3 windowOffset = glm::vec3(0.0f, 0.0f, 0.0f);
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.05f, 4.0f, 1.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(6.0f, 5.0f, 1.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.5f, 0.01f, 0.01f, 1.0f);
		//SetShaderTexture("rfab");
		//SetTextureUVScale(0.5f, 0.5f); 

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	
	{
		
		// --- 1. DEFINE THE MAIN NOREN BODY ---
		glm::vec3 mainScale = glm::vec3(5.00f, 1.5f, 0.02f);
		glm::vec3 mainPos = glm::vec3(4.0f, 6.0f, -10.5f);
		float mainWidth = mainScale.x;
		float mainHeight = mainScale.y;

		// Set and Draw the Main Rectangle
		SetTransformations(mainScale, 0.0f, 0.0f, 0.0f, mainPos);
		SetShaderTexture("rfab");
		SetTextureUVScale(1.0f, 0.25f);
		m_basicMeshes->DrawBoxMesh();


		// --- 2. DEFINE AND DRAW THE LOOPS (TABS) ---

		int numLoops = 6;
		float loopWidth = 0.6f;
		float loopHeight = 0.3f;
		float tabDepth = 0.02f;

		// tab spacing
		float spacing = (mainWidth - loopWidth) / (float)(numLoops - 1);

		// Start the first tab at the far left edge of the body
		float startX = (mainPos.x - (mainWidth / 2.0f)) + (loopWidth / 2.0f);

		for (int i = 0; i < numLoops; i++) {
			// Calculate X position Move from the starting point by the spacing increment
			float currentTabX = startX + (i * spacing);

			// Calculate Y position: Top of main box + half height of tab
			float currentTabY = mainPos.y + (mainHeight / 2.0f) + (loopHeight / 2.0f);

			glm::vec3 loopPos = glm::vec3(currentTabX, currentTabY, mainPos.z);
			glm::vec3 loopScale = glm::vec3(loopWidth, loopHeight, tabDepth);

			SetTransformations(
				loopScale,
				0.0f,
				0.0f,
				0.0f,
				loopPos
			);

			// Use the same texture/UVs as the main body
			SetShaderTexture("rfab");
			SetTextureUVScale(0.25f, 0.25f);

			m_basicMeshes->DrawBoxMesh();
		}
	}
	{
		// 1. Main Box Setup
		glm::vec3 mainScale2 = glm::vec3(5.00f, 1.5f, 0.02f);
		glm::vec3 mainPos2 = glm::vec3(-4.0f, 7.0f, 1.5f);
		float mainWidth2 = mainScale2.x;
		float mainHeight2 = mainScale2.y;

		// Rotation 90 deg on Y: The box's X-axis (width) now aligns with the World Z-axis
		SetTransformations(mainScale2, 0.0f, 90.0f, 0.0f, mainPos2);
		SetShaderTexture("rfab");
		SetTextureUVScale(1.0f, 0.25f);
		m_basicMeshes->DrawBoxMesh();

		// 2. Tab Setup
		int numLoops2 = 10;
		float loopWidth2 = 0.15f;  // This is the "length" of the tab along the Z axis (due to rotation)
		float loopHeight2 = 0.3f;
		float tabDepth2 = 0.02f;

		// The spacing between centers is the total available width minus one tab width, 
		// divided by the number of gaps.
		float spacing2 = (mainWidth2 - loopWidth2) / (float)(numLoops2 - 1);

		// calculate center for offset
		float firstTabCenterZ = (mainPos2.z - (mainWidth2 / 2.0f)) + (loopWidth2 / 2.0f);

		for (int i = 0; i < numLoops2; i++) {
			// Calculate the Z position by starting at the first center and adding increments
			float currentTabZ = firstTabCenterZ + (i * spacing2);

			// The Y position: Top of the box + half the height of the tab
			float currentTabY2 = mainPos2.y + (mainHeight2 / 2.0f) + (loopHeight2 / 2.0f);

			// X remains aligned with the main box center
			glm::vec3 loopPos2 = glm::vec3(mainPos2.x, currentTabY2, currentTabZ);
			glm::vec3 loopScale2 = glm::vec3(loopWidth2, loopHeight2, tabDepth2);

			// rotate the tabs 90 degrees so their width (X) aligns with World Z
			SetTransformations(loopScale2, 0.0f, 90.0f, 0.0f, loopPos2);
			SetShaderTexture("rfab");
			SetTextureUVScale(0.25f, 0.25f);
			m_basicMeshes->DrawBoxMesh();
		}
	}
	{
		// 1. Main Box Setup
		glm::vec3 mainScale2 = glm::vec3(1.00f, 4.0f, 0.02f);
		glm::vec3 mainPos2 = glm::vec3(-3.9f, 8.5f, 1.5f);
		float mainWidth2 = mainScale2.x;
		float mainHeight2 = mainScale2.y;

		// Rotation 90 deg on Y: The box's X-axis (width) now aligns with the World Z-axis
		SetTransformations(mainScale2, 0.0f, 90.0f, 0.0f, mainPos2);
		SetShaderTexture("rfab");
		SetTextureUVScale(1.0f, 0.25f);
		m_basicMeshes->DrawBoxMesh();

		// 2. Tab Setup
		int numLoops2 = 2;
		float loopWidth2 = 0.15f;  // "length" of the tab along the Z axis (due to rotation)
		float loopHeight2 = 0.3f;
		float tabDepth2 = 0.02f;

		// The spacing between centers is the total available width minus one tab width, 
		// divided by the number of gaps.
		float spacing2 = (mainWidth2 - loopWidth2) / (float)(numLoops2 - 1);

		// calculate center for offset
		float firstTabCenterZ = (mainPos2.z - (mainWidth2 / 2.0f)) + (loopWidth2 / 2.0f);

		for (int i = 0; i < numLoops2; i++) {
			// Calculate the Z position by starting at the first center and adding increments
			float currentTabZ = firstTabCenterZ + (i * spacing2);

			// The Y position: Top of the box + half the height of the tab
			float currentTabY2 = mainPos2.y + (mainHeight2 / 2.0f) + (loopHeight2 / 2.0f);

			// X remains aligned with the main box center
			glm::vec3 loopPos2 = glm::vec3(mainPos2.x, currentTabY2, currentTabZ);
			glm::vec3 loopScale2 = glm::vec3(loopWidth2, loopHeight2, tabDepth2);

			// rotate the tabs 90 degrees so their width (X) aligns with World Z
			SetTransformations(loopScale2, 0.0f, 90.0f, 0.0f, loopPos2);
			SetShaderTexture("rfab");
			SetTextureUVScale(0.25f, 0.25f);
			m_basicMeshes->DrawBoxMesh();
		}
	}
	{// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(1.0f, 2.0f, 0.05f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(0.0f, 6.0f, -13.25f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.5f, 0.01f, 0.01f, 1.0f);
		//SetShaderTexture("rfab");
		//SetTextureUVScale(0.5f, 0.5f); 

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	{// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(1.0f, 3.0f, 0.05f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-3.5f, 13.0f, -2.5f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.5f, 0.01f, 0.01f, 1.0f);
		//SetShaderTexture("rfab");
		//SetTextureUVScale(0.5f, 0.5f); 

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	{// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(1.0f, 1.0f, 0.1f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-3.0f, 14.0f, -2.3f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.5f, 0.01f, 0.01f, 1.0f);
		//SetShaderTexture("rfab");
		//SetTextureUVScale(0.5f, 0.5f); 

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	{
		// 1. Main Box Setup (No rotation, so Width = X)
		glm::vec3 mainScale2 = glm::vec3(1.00f, 4.0f, 0.02f);
		glm::vec3 mainPos2 = glm::vec3(-3.9f, 14.5f, 1.5f);
		float mainWidth2 = mainScale2.x;   // This is 1.0
		float mainHeight2 = mainScale2.y;  // This is 4.0

		// Rotation is 0, so the box sits flat on the XZ plane
		SetTransformations(mainScale2, 0.0f, 0.0f, 0.0f, mainPos2);
		SetShaderTexture("rfab");
		SetTextureUVScale(1.0f, 0.25f);
		m_basicMeshes->DrawBoxMesh();

		// 2. Tab Setup
		int numLoops2 = 2;
		float loopWidth2 = 0.15f;
		float loopHeight2 = 0.3f;
		float tabDepth2 = 0.02f;

		// Spacing calculation remains the same, but applies to X now
		float spacing2 = (mainWidth2 - loopWidth2) / (float)(numLoops2 - 1);

		// Calculate center for X axis instead of Z axis
		// We move from the left edge (mainPos.x - halfWidth) inward by half a tab width
		float firstTabCenterX = (mainPos2.x - (mainWidth2 / 2.0f)) + (loopWidth2 / 2.0f);

		for (int i = 0; i < numLoops2; i++) {
			// Increment along the X axis
			float currentTabX = firstTabCenterX + (i * spacing2);

			// The Y position: Top of the box + half the height of the tab
			float currentTabY2 = mainPos2.y + (mainHeight2 / 2.0f) + (loopHeight2 / 2.0f);

			// Keep Z constant at the box's center Z, but use our new X position
			glm::vec3 loopPos2 = glm::vec3(currentTabX, currentTabY2, mainPos2.z);
			glm::vec3 loopScale2 = glm::vec3(loopWidth2, loopHeight2, tabDepth2);

			SetTransformations(loopScale2, 0.0f, 0.0f, 0.0f, loopPos2);
			SetShaderTexture("rfab");
			SetTextureUVScale(0.25f, 0.25f);
			m_basicMeshes->DrawBoxMesh();
		}
	}
	{// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(1.0f, 3.0f, 0.05f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-5.25f, 5.0f, 1.5f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		SetShaderColor(0.5f, 0.01f, 0.01f, 1.0f);
		//SetShaderTexture("rfab");
		//SetTextureUVScale(0.5f, 0.5f); 

		// draw the mesh with transformation values
		m_basicMeshes->DrawBoxMesh();
	}
	{
		// 1. Main Box Setup
		glm::vec3 mainScale2 = glm::vec3(5.00f, 1.5f, 0.02f);
		glm::vec3 mainPos2 = glm::vec3(-4.0f, 7.0f, 7.5f);
		float mainWidth2 = mainScale2.x;
		float mainHeight2 = mainScale2.y;

		// Rotation 90 deg on Y: The box's X-axis (width) now aligns with the World Z-axis
		SetTransformations(mainScale2, 0.0f, 90.0f, 0.0f, mainPos2);
		SetShaderTexture("rfab");
		SetTextureUVScale(1.0f, 0.25f);
		m_basicMeshes->DrawBoxMesh();

		// 2. Tab Setup
		int numLoops2 = 10;
		float loopWidth2 = 0.15f;  // This is the "length" of the tab along the Z axis (due to rotation)
		float loopHeight2 = 0.3f;
		float tabDepth2 = 0.02f;

		// The spacing between centers is the total available width minus one tab width, 
		// divided by the number of gaps.
		float spacing2 = (mainWidth2 - loopWidth2) / (float)(numLoops2 - 1);

		// calculate center for offset
		float firstTabCenterZ = (mainPos2.z - (mainWidth2 / 2.0f)) + (loopWidth2 / 2.0f);

		for (int i = 0; i < numLoops2; i++) {
			// Calculate the Z position by starting at the first center and adding increments
			float currentTabZ = firstTabCenterZ + (i * spacing2);

			// The Y position: Top of the box + half the height of the tab
			float currentTabY2 = mainPos2.y + (mainHeight2 / 2.0f) + (loopHeight2 / 2.0f);

			// X remains aligned with the main box center
			glm::vec3 loopPos2 = glm::vec3(mainPos2.x, currentTabY2, currentTabZ);
			glm::vec3 loopScale2 = glm::vec3(loopWidth2, loopHeight2, tabDepth2);

			// rotate the tabs 90 degrees so their width (X) aligns with World Z
			SetTransformations(loopScale2, 0.0f, 90.0f, 0.0f, loopPos2);
			SetShaderTexture("rfab");
			SetTextureUVScale(0.25f, 0.25f);
			m_basicMeshes->DrawBoxMesh();
		}
	}
	{// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.8f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-1.25f, 15.0f, -1.5f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		//SetShaderColor(0.5f, 0.01f, 0.01f, 1.0f);
		SetShaderTexture("pmetal");
		SetTextureUVScale(1.0f, 30.0f); 

		// draw the mesh with transformation values
		m_basicMeshes->DrawSphereMesh();
	}
	{// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(1.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-7.25f, 12.0f, -18.5f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		//SetShaderColor(0.5f, 0.01f, 0.01f, 1.0f);
		SetShaderTexture("pmetal");
		SetTextureUVScale(1.0f, 30.0f);

		// draw the mesh with transformation values
		m_basicMeshes->DrawSphereMesh();
	}
	{// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(0.8f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(-4.25f, 8.0f, -25.5f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		//SetShaderColor(0.5f, 0.01f, 0.01f, 1.0f);
		SetShaderTexture("pmetal");
		SetTextureUVScale(1.0f, 30.0f);

		// draw the mesh with transformation values
		m_basicMeshes->DrawSphereMesh();
	}
}
	