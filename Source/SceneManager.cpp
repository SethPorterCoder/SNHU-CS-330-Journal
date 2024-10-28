///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
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

	//Sunset vibe rather than the disco red-blue vibe from last assignment


	// Mimic the setting sun reddish orange hue
	m_directionalLight1.direction = glm::vec3(-2.4f, -1.0f, -0.3f);  // Lower sun angle for a warm sunset
	m_directionalLight1.ambient = glm::vec3(0.8f, 0.4f, 0.2f);       // Stronger red-orange ambient light
	m_directionalLight1.diffuse = glm::vec3(1.0f, 0.5f, 0.3f);       // Warm reddish-orange diffuse light
	m_directionalLight1.specular = glm::vec3(1.0f, 0.6f, 0.4f);      // Warm reddish specular highlights
	m_directionalLight1.bActive = true;

	// Mimic the rising twilight cool blue hue
	m_directionalLight2.direction = glm::vec3(2.2f, -0.5f, -0.4f);   // Softer angle, opposite to the sunset
	m_directionalLight2.ambient = glm::vec3(0.2f, 0.3f, 0.7f);       // Softer blue-purple ambient light
	m_directionalLight2.diffuse = glm::vec3(0.3f, 0.4f, 0.8f);       // Stronger blue diffuse light to balance the scene
	m_directionalLight2.specular = glm::vec3(0.5f, 0.6f, 1.0f);      // Cool blue specular highlights
	m_directionalLight2.bActive = true;


	




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

void SceneManager::LoadSceneTextures()
{
	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene. Refer to the code in   ***/
	/*** the OpenGL Sample for help.                                 ***/

	if (!CreateGLTexture("textures/deskTop.jpg", "deskTop"))
	{
		std::cout << "Failed to load desktop.jpg texture!" << std::endl;
	}

	if (!CreateGLTexture("textures/deskRod.jpg", "deskRod"))
	{
		std::cout << "Failed to load deskRod.jpg texture!" << std::endl;
	}

	if (!CreateGLTexture("textures/deskRim.jpg", "deskRim"))
	{
		std::cout << "Failed to load deskRim.jpg texture!" << std::endl;
	}

	if (!CreateGLTexture("textures/granite.jpg", "quartz"))
	{
		std::cout << "Failed to load granite.jpg texture!" << std::endl;
	}

	if (!CreateGLTexture("textures/copper.jpg", "copper"))
	{
		std::cout << "Failed to load copper.jpg texture!" << std::endl;
	}

	if (!CreateGLTexture("textures/pencil.jpg", "pencil"))
	{
		std::cout << "Failed to load pencil.jpg texture!" << std::endl;
	}

	if (!CreateGLTexture("textures/erase.jpg", "erase"))
	{
		std::cout << "Failed to load erase.jpg texture!" << std::endl;
	}
	if (!CreateGLTexture("textures/grain.jpg", "grain"))
	{
		std::cout << "Failed to load grain.jpg texture!" << std::endl;
	}


	
	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
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


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()

// load the textures for the 3D scene
{
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	LoadSceneTextures();


	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
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






	m_pShaderManager->setBoolValue("bUseLighting", true);

	// Pass the first directional light to the shader
	m_pShaderManager->setVec3Value("directionalLight1.direction", m_directionalLight1.direction);
	m_pShaderManager->setVec3Value("directionalLight1.ambient", m_directionalLight1.ambient);
	m_pShaderManager->setVec3Value("directionalLight1.diffuse", m_directionalLight1.diffuse);
	m_pShaderManager->setVec3Value("directionalLight1.specular", m_directionalLight1.specular);
	m_pShaderManager->setBoolValue("directionalLight1.bActive", m_directionalLight1.bActive);

	// Pass the second directional light to the shader
	m_pShaderManager->setVec3Value("directionalLight2.direction", m_directionalLight2.direction);
	m_pShaderManager->setVec3Value("directionalLight2.ambient", m_directionalLight2.ambient);
	m_pShaderManager->setVec3Value("directionalLight2.diffuse", m_directionalLight2.diffuse);
	m_pShaderManager->setVec3Value("directionalLight2.specular", m_directionalLight2.specular);
	m_pShaderManager->setBoolValue("directionalLight2.bActive", m_directionalLight2.bActive);

	// Pass the blue point light to the shader
	m_pShaderManager->setVec3Value("pointLights[0].position", m_pointLight1.position);
	m_pShaderManager->setVec3Value("pointLights[0].ambient", m_pointLight1.ambient);
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", m_pointLight1.diffuse);
	m_pShaderManager->setVec3Value("pointLights[0].specular", m_pointLight1.specular);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", m_pointLight1.bActive);

	// Pass the red point light to the shader
	m_pShaderManager->setVec3Value("pointLights[1].position", m_pointLight2.position);
	m_pShaderManager->setVec3Value("pointLights[1].ambient", m_pointLight2.ambient);
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", m_pointLight2.diffuse);
	m_pShaderManager->setVec3Value("pointLights[1].specular", m_pointLight2.specular);
	m_pShaderManager->setBoolValue("pointLights[1].bActive", m_pointLight2.bActive);

	// Pass the spotlight to the shader
	m_pShaderManager->setVec3Value("spotLight.position", m_spotLight.position);
	m_pShaderManager->setVec3Value("spotLight.direction", m_spotLight.direction);
	m_pShaderManager->setFloatValue("spotLight.cutOff", m_spotLight.cutOff);
	m_pShaderManager->setFloatValue("spotLight.outerCutOff", m_spotLight.outerCutOff);
	m_pShaderManager->setFloatValue("spotLight.constant", m_spotLight.constant);
	m_pShaderManager->setFloatValue("spotLight.linear", m_spotLight.linear);
	m_pShaderManager->setFloatValue("spotLight.quadratic", m_spotLight.quadratic);
	m_pShaderManager->setVec3Value("spotLight.ambient", m_spotLight.ambient);
	m_pShaderManager->setVec3Value("spotLight.diffuse", m_spotLight.diffuse);
	m_pShaderManager->setVec3Value("spotLight.specular", m_spotLight.specular);
	m_pShaderManager->setBoolValue("spotLight.bActive", m_spotLight.bActive);









	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(30.0f, 1.0f, 30.0f);

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

	SetShaderColor(1, 1, 1, 1);

	// draw the mesh with transformation values
	SetShaderTexture("quartz");




	Material shinyMaterial;
	shinyMaterial.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);  // Base white color
	shinyMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f); // Strong white specular highlights
	shinyMaterial.shininess = 128.0f;  // Very shiny

	// Pass the material to the shader
	m_pShaderManager->setVec3Value("material.diffuseColor", shinyMaterial.diffuseColor);
	m_pShaderManager->setVec3Value("material.specularColor", shinyMaterial.specularColor);
	m_pShaderManager->setFloatValue("material.shininess", shinyMaterial.shininess);




	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	//Lower box for desk
	XrotationDegrees = 90.0f;
	scaleXYZ = glm::vec3(30.0f, 1.0f, 30.0f);
	positionXYZ = glm::vec3(0.0f, 30.0f, -30.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1, 1, 1, 1);  // Brown color
	SetTextureUVScale(1.0f, 1.0f);  // Apply tiling to deskRod texture (adjust tiling as needed)



	SetShaderTexture("quartz");
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/



	// Material for non-reflective objects
	Material nonReflectiveMaterial;
	nonReflectiveMaterial.diffuseColor = glm::vec3(0.65f, 0.16f, 0.16f);  // Brown color
	nonReflectiveMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);    // Low reflectivity
	nonReflectiveMaterial.shininess = 16.0f;                              // Low shininess

	// Pass the non-reflective material to the shader
	m_pShaderManager->setVec3Value("material.diffuseColor", nonReflectiveMaterial.diffuseColor);
	m_pShaderManager->setVec3Value("material.specularColor", nonReflectiveMaterial.specularColor);
	m_pShaderManager->setFloatValue("material.shininess", nonReflectiveMaterial.shininess);


	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	//Lower box for desk
	scaleXYZ = glm::vec3(25.0f, 2.0f, 15.0f);
	positionXYZ = glm::vec3(0.0f, 10.0f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.65f, 0.16f, 0.16f, 1);  // Brown color
	SetTextureUVScale(1.0f, 1.0f);  // Apply tiling to deskRod texture (adjust tiling as needed)
	SetShaderTexture("deskRim");
	m_basicMeshes->DrawBoxMesh();

	//Top box for desk
	scaleXYZ = glm::vec3(25.5f, 0.5f, 15.5f);
	positionXYZ = glm::vec3(0.0f, 11.0f, 0.0f); 
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.65f, 0.16f, 0.16f, 1);  // Brown color
	SetShaderTexture("deskTop");
	m_basicMeshes->DrawBoxMesh();


	//Seting the reflective materials again
	m_pShaderManager->setVec3Value("material.diffuseColor", shinyMaterial.diffuseColor);
	m_pShaderManager->setVec3Value("material.specularColor", shinyMaterial.specularColor);
	m_pShaderManager->setFloatValue("material.shininess", shinyMaterial.shininess);

	//Right leg backward
	scaleXYZ = glm::vec3(1.0f, 10.0f, 1.0f);
	positionXYZ = glm::vec3(10.0f, 0.0f, -5.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.5f, 0.5f, 0.5f, 1);  // Grey color
	SetShaderTexture("deskRod");
	SetTextureUVScale(2.0f, 2.0f);  // Apply tiling to deskRod texture (adjust tiling as needed)
	m_basicMeshes->DrawCylinderMesh();

	//Right leg forward
	scaleXYZ = glm::vec3(1.0f, 10.0f, 1.0f);
	positionXYZ = glm::vec3(10.0f, 0.0f, 5.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.5f, 0.5f, 0.5f, 1);  // Grey color
	SetShaderTexture("deskRod");
	SetTextureUVScale(2.0f, 2.0f);  // Apply tiling to deskRod texture
	m_basicMeshes->DrawCylinderMesh();

	//Left leg backward
	scaleXYZ = glm::vec3(1.0f, 10.0f, 1.0f);
	positionXYZ = glm::vec3(-10.0f, 0.0f, -5.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.5f, 0.5f, 0.5f, 1);  // Grey color
	SetShaderTexture("deskRod");
	SetTextureUVScale(2.0f, 2.0f);  // Apply tiling to deskRod texture
	m_basicMeshes->DrawCylinderMesh();

	//Left leg forward
	scaleXYZ = glm::vec3(1.0f, 10.0f, 1.0f);
	positionXYZ = glm::vec3(-10.0f, 0.0f, 5.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.5f, 0.5f, 0.5f, 1);  // Grey color
	SetShaderTexture("deskRod");
	SetTextureUVScale(2.0f, 2.0f);  // Apply tiling to deskRod texture
	m_basicMeshes->DrawCylinderMesh();

	//Left leg bracer
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	scaleXYZ = glm::vec3(1.0f, 10.0f, 1.0f);
	positionXYZ = glm::vec3(-10.0f, 5.0f, -5.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.5f, 0.5f, 0.5f, 1);  // Grey color
	SetShaderTexture("deskRod");
	SetTextureUVScale(2.0f, 2.0f);  // Apply tiling to deskRod texture
	m_basicMeshes->DrawCylinderMesh();

	//Right leg bracer
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	scaleXYZ = glm::vec3(1.0f, 10.0f, 1.0f);
	positionXYZ = glm::vec3(10.0f, 5.0f, -5.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.5f, 0.5f, 0.5f, 1);  // Grey color
	SetShaderTexture("deskRod");
	SetTextureUVScale(2.0f, 2.0f);  // Apply tiling to deskRod texture
	m_basicMeshes->DrawCylinderMesh();

	//Lamp base
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	scaleXYZ = glm::vec3(2.0f, 1.0f, 2.0f);
	positionXYZ = glm::vec3(8.0f, 11.0f, -5.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.5f, 0.5f, 0.5f, 1);  // Grey color
	SetShaderTexture("copper");
	SetTextureUVScale(1, 1); 
	m_basicMeshes->DrawCylinderMesh();

	//Lamp base top
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	scaleXYZ = glm::vec3(2.0f, 1.0f, 2.0f);
	positionXYZ = glm::vec3(8.0f, 12.0f, -5.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.5f, 0.5f, 0.5f, 1);  // Grey color
	SetShaderTexture("copper");
	SetTextureUVScale(1, 1);	
	m_basicMeshes->DrawSphereMesh();

	// Lamp bottom pipe connect bottom
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	scaleXYZ = glm::vec3(0.5f, 1.0f, 0.5f);
	positionXYZ = glm::vec3(8.0f, 12.5f, -5.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.5f, 0.5f, 0.5f, 1);  // Grey color
	SetShaderTexture("copper");
	SetTextureUVScale(1, 1);
	m_basicMeshes->DrawCylinderMesh();

	// Lamp bottom pipe
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -15.0f;

	scaleXYZ = glm::vec3(0.25f, 7.5f, 0.25f);
	positionXYZ = glm::vec3(7.75f, 12.5f, -5.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.5f, 0.5f, 0.5f, 1);  // Grey color
	SetShaderTexture("copper");
	SetTextureUVScale(1, 1);
	m_basicMeshes->DrawCylinderMesh();

	//Lamp bottom pipe connect top
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -15.0f;

	scaleXYZ = glm::vec3(0.5f, 0.5f, 0.5f);
	positionXYZ = glm::vec3(9.65f, 19.5f, -5.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.5f, 0.5f, 0.5f, 1);  // Grey color
	SetShaderTexture("copper");
	SetTextureUVScale(1, 1);
	m_basicMeshes->DrawCylinderMesh();

	// Lamp joint
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	scaleXYZ = glm::vec3(0.65f, 0.65f, 0.65f);
	positionXYZ = glm::vec3(9.80f, 20.25f, -5.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.5f, 0.5f, 0.5f, 1);  // Grey color
	SetShaderTexture("copper");
	SetTextureUVScale(1, 1);
	m_basicMeshes->DrawSphereMesh();

	// Top Rod
	XrotationDegrees = 45.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	scaleXYZ = glm::vec3(0.25f, 7.5f, 0.25f);
	positionXYZ = glm::vec3(9.80f, 20.25f, -5.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.5f, 0.5f, 0.5f, 1);  // Grey color
	SetShaderTexture("copper");
	SetTextureUVScale(1, 1);
	m_basicMeshes->DrawCylinderMesh();

	// Base Shell
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	scaleXYZ = glm::vec3(1.0f, 1.5f, 1.0f);
	positionXYZ = glm::vec3(4.0f, 19.5f, 0.5f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.5f, 0.5f, 0.5f, 1);  // Grey color
	SetShaderTexture("copper");
	SetTextureUVScale(1, 1);
	m_basicMeshes->DrawCylinderMesh();

	//Light Base Shell
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	scaleXYZ = glm::vec3(1.5f, 1.0f, 1.5f);
	positionXYZ = glm::vec3(4.0f, 19.0f, 0.5f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.5f, 0.5f, 0.5f, 1);  // Grey color
	SetShaderTexture("copper");
	SetTextureUVScale(1, 1);
	m_basicMeshes->DrawTaperedCylinderMesh();

	// Pass the non-reflective material to the shader again
	m_pShaderManager->setVec3Value("material.diffuseColor", nonReflectiveMaterial.diffuseColor);
	m_pShaderManager->setVec3Value("material.specularColor", nonReflectiveMaterial.specularColor);
	m_pShaderManager->setFloatValue("material.shininess", nonReflectiveMaterial.shininess);

	//Paper
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	scaleXYZ = glm::vec3(5.0f, 0.05f, 5.0f);
	positionXYZ = glm::vec3(0.0f, 11.25f, 2.5f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1);  // Brown color
	//SetShaderTexture("deskTop");
	m_basicMeshes->DrawBoxMesh();

	//pencil rod
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	scaleXYZ = glm::vec3(0.10f, 2.0f, 0.10f);
	positionXYZ = glm::vec3(5.0f, 11.35f, 2.5f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 0.6f, 0.2f, 1);  // Yellow-orange pencil color
	m_basicMeshes->DrawCylinderMesh();

	//Pencil wood before tip
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	scaleXYZ = glm::vec3(0.10f, 0.08f, 0.10f);
	positionXYZ = glm::vec3(5.0f, 11.35f, 4.5f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.55f, 0.27f, 0.07f, 1);  // Brown wood color before the pencil tip	
	m_basicMeshes->DrawTaperedCylinderMesh();

	//Pencil tip
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	scaleXYZ = glm::vec3(0.06f, 0.2f, 0.05f);
	positionXYZ = glm::vec3(5.0f, 11.35f, 4.58f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1);  // Black
	//SetShaderTexture("deskTop");
	m_basicMeshes->DrawConeMesh();

	//Pencil eraser
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	scaleXYZ = glm::vec3(0.10f, 0.25f, 0.10f);
	positionXYZ = glm::vec3(5.0f, 11.35f, 2.25f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1);  // Yellow-orange pencil color
	SetShaderTexture("erase");
	SetTextureUVScale(1, 1);
	m_basicMeshes->DrawCylinderMesh();

}
