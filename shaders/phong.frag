#version 410

in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

#define MAX_LIGHTS 10

struct LightInfo {
  vec4 Position; // Light position in eye coords.
  vec3 La;       // Ambient light intensity
  vec3 Ld;       // Diffuse light intensity
  vec3 Ls;       // Specular light intensity
};
uniform LightInfo lights[MAX_LIGHTS];
uniform int numLights;

struct MaterialInfo {
  vec3 Ka;            // Ambient reflectivity
  vec3 Kd;            // Diffuse reflectivity
  vec3 Ks;            // Specular reflectivity
  float Shininess;    // Specular shininess factor
};
uniform MaterialInfo Material;

uniform bool blinn;

vec3 calculateLight(int index, vec3 n, vec3 v) {
    vec3 s = normalize(vec3(lights[index].Position) - FragPos);
    vec3 halfwayDir = normalize(s + v);    
    vec3 ambient = lights[index].La * Material.Ka;
    
    float sDotN = max(dot(s, n), 0.0);
    vec3 diffuse = lights[index].Ld * Material.Kd * sDotN;
    
    vec3 spec = vec3(0.0);
    if(sDotN > 0.0) {
        float specFactor = pow(max(dot(n, halfwayDir), 0.0), Material.Shininess);
        spec = lights[index].Ls * Material.Ks * specFactor;
    }
    return ambient + diffuse + spec;
}

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
  
    vec3 La;
    vec3 Ld;
    vec3 Ls;
    
    float constant;
    float linear;
    float quadratic;
};

uniform SpotLight flashlight;
uniform bool flashlightOn;

vec3 calculateSpotLight(SpotLight light, vec3 n, vec3 v) {
    vec3 lightDir = normalize(light.position - FragPos);
    
    // Diffuse shading
    float diff = max(dot(n, lightDir), 0.0);
    
    // Specular shading
    vec3 halfwayDir = normalize(lightDir + v);
    float spec = pow(max(dot(n, halfwayDir), 0.0), Material.Shininess);
    
    // Attenuation
    float distance = length(light.position - FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    
    // Spotlight intensity
    float theta = dot(lightDir, normalize(-light.direction)); 
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    
    // Combine results
    vec3 ambient = light.La * Material.Ka;
    vec3 diffuse = light.Ld * diff * Material.Kd;
    vec3 specular = light.Ls * spec * Material.Ks;
    
    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    
    return (ambient + diffuse + specular);
}

void main() {
    vec3 n = normalize(Normal);
    vec3 v = normalize(-FragPos);
    
    vec3 result = vec3(0.0);
    
    for(int i = 0; i < numLights; ++i) {
        result += calculateLight(i, n, v);
    }
    
    if(flashlightOn) {
        result += calculateSpotLight(flashlight, n, v);
    }
    
    FragColor = vec4(result, 1.0);
}