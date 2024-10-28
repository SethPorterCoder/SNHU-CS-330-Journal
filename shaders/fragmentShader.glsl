#version 330 core
out vec4 fragmentColor;

in vec3 fragmentPosition;
in vec3 fragmentVertexNormal;
in vec2 fragmentTextureCoordinate;

struct Material {
    vec3 diffuseColor;
    vec3 specularColor;
    float shininess;
}; 

struct DirectionalLight {
    vec3 direction;
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    bool bActive;
};

struct PointLight {
    vec3 position;
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    bool bActive;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    bool bActive;
};

// Update to handle four point lights
#define TOTAL_POINT_LIGHTS 4
#define TOTAL_SPOT_LIGHTS 5

uniform bool bUseTexture = false;
uniform bool bUseLighting = false;
uniform vec4 objectColor = vec4(1.0f);
uniform vec3 viewPosition;

// Two directional lights
uniform DirectionalLight directionalLight1;
uniform DirectionalLight directionalLight2;

uniform PointLight pointLights[TOTAL_POINT_LIGHTS];
uniform SpotLight spotLights[TOTAL_SPOT_LIGHTS];
uniform Material material;
uniform sampler2D objectTexture;
uniform vec2 UVscale = vec2(1.0f, 1.0f);

// function prototypes
vec3 CalcDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

void main()
{
    vec3 norm = normalize(fragmentVertexNormal);
    vec3 viewDir = normalize(viewPosition - fragmentPosition);

    vec3 lightingResult = vec3(0.0f);

    if(bUseLighting == true)
    {
        // Phase 1: directional lighting (two lights)
        if(directionalLight1.bActive == true)
        {
            lightingResult += CalcDirectionalLight(directionalLight1, norm, viewDir);
        }
        if(directionalLight2.bActive == true)
        {
            lightingResult += CalcDirectionalLight(directionalLight2, norm, viewDir);
        }

        // Phase 2: point lights (now processing four lights)
        for(int i = 0; i < TOTAL_POINT_LIGHTS; i++)
        {
            if(pointLights[i].bActive == true)
            {
                lightingResult += CalcPointLight(pointLights[i], norm, fragmentPosition, viewDir);   
            }
        }

        // Phase 3: spotlights
        for(int i = 0; i < TOTAL_SPOT_LIGHTS; i++)
        {
            if(spotLights[i].bActive == true)
            {
                lightingResult += CalcSpotLight(spotLights[i], norm, fragmentPosition, viewDir);    
            }
        }
    }

    // Final color calculation with or without texture
    vec3 baseColor;
    if(bUseTexture == true)
    {
        baseColor = vec3(texture(objectTexture, fragmentTextureCoordinate * UVscale));
    }
    else
    {
        baseColor = vec3(objectColor);
    }

    // Mix lighting result with the base color
    vec3 finalColor = lightingResult * baseColor;

    fragmentColor = vec4(finalColor, 1.0f);  // Alpha is set to 1 for solid objects
}

// Calculates the color when using a directional light.
vec3 CalcDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    
    // Ambient
    vec3 ambient = light.ambient * (bUseTexture ? vec3(texture(objectTexture, fragmentTextureCoordinate)) : vec3(objectColor));

    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * material.diffuseColor;

    // Specular (stronger specular for reflective surfaces)
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * material.specularColor;

    return (ambient + diffuse + specular);  // Return specular lighting result as part of the light
}

// Calculates the color when using a point light.
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);

    // Ambient
    vec3 ambient = light.ambient * (bUseTexture ? vec3(texture(objectTexture, fragmentTextureCoordinate)) : vec3(objectColor));

    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * material.diffuseColor;

    // Specular
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * material.specularColor;

    return (ambient + diffuse + specular);
}

// Calculates the color when using a spotlight.
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);

    // Diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * material.diffuseColor;

    // Specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * material.specularColor;

    // Attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    // Spotlight intensity
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    // Combine results
    vec3 ambient = light.ambient * (bUseTexture ? vec3(texture(objectTexture, fragmentTextureCoordinate)) : vec3(objectColor));
    diffuse *= intensity;
    specular *= intensity;

    return (ambient + diffuse + specular) * attenuation;
}
