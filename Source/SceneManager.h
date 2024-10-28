///////////////////////////////////////////////////////////////////////////////
// scenemanager.h
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//  Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ShaderManager.h"
#include "ShapeMeshes.h"

#include <string>
#include <vector>

/***********************************************************
 *  SceneManager
 *
 *  This class contains the code for preparing and rendering
 *  3D scenes, including the shader settings.
 ***********************************************************/
class SceneManager
{
public:
    // constructor
    SceneManager(ShaderManager* pShaderManager);
    // destructor
    ~SceneManager();

    struct TEXTURE_INFO
    {
        std::string tag;
        uint32_t ID;
    };

    struct OBJECT_MATERIAL
    {
        glm::vec3 diffuseColor;
        glm::vec3 specularColor;
        float shininess;
        std::string tag;
    };

    // Used for reflectivity in this assignment
    struct Material
    {
        glm::vec3 diffuseColor;
        glm::vec3 specularColor;
        float shininess;
    };

    // Structure for a directional light
    struct DirectionalLight {
        glm::vec3 direction;

        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;

        bool bActive;
    };

    // Structure for a point light
    struct PointLight {
        glm::vec3 position;

        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;

        bool bActive;
    };

    // Structure for a spotlight
    struct SpotLight {
        glm::vec3 position;
        glm::vec3 direction;
        float cutOff;
        float outerCutOff;

        float constant;
        float linear;
        float quadratic;

        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;

        bool bActive;
    };

private:
    // pointer to shader manager object
    ShaderManager* m_pShaderManager;
    // pointer to basic shapes object
    ShapeMeshes* m_basicMeshes;
    // total number of loaded textures
    int m_loadedTextures;
    // loaded textures info
    TEXTURE_INFO m_textureIDs[16];
    // defined object materials
    std::vector<OBJECT_MATERIAL> m_objectMaterials;

    DirectionalLight m_directionalLight1;  // First directional light
    DirectionalLight m_directionalLight2;  // Second directional light
    PointLight m_pointLight1;              // First point light (blue)
    PointLight m_pointLight2;              // Second point light (red)
    SpotLight m_spotLight;                 // Spotlight

    // load texture images and convert to OpenGL texture data
    bool CreateGLTexture(const char* filename, std::string tag);
    // bind loaded OpenGL textures to slots in memory
    void BindGLTextures();
    // free the loaded OpenGL textures
    void DestroyGLTextures();
    // find a loaded texture by tag
    int FindTextureID(std::string tag);
    int FindTextureSlot(std::string tag);
    // find a defined material by tag
    bool FindMaterial(std::string tag, OBJECT_MATERIAL& material);

    void LoadSceneTextures();

    // set the transformation values 
    // into the transform buffer
    void SetTransformations(
        glm::vec3 scaleXYZ,
        float XrotationDegrees,
        float YrotationDegrees,
        float ZrotationDegrees,
        glm::vec3 positionXYZ);

    // set the color values into the shader
    void SetShaderColor(
        float redColorValue,
        float greenColorValue,
        float blueColorValue,
        float alphaValue);

    // set the texture data into the shader
    void SetShaderTexture(
        std::string textureTag);

    // set the UV scale for the texture mapping
    void SetTextureUVScale(
        float u, float v);

    // set the object material into the shader
    void SetShaderMaterial(
        std::string materialTag);

public:
    // The following methods are for the students to 
    // customize for their own 3D scene
    void PrepareScene();
    void RenderScene();
};
