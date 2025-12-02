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
    
    vec3 ambient = lights[index].La * Material.Ka;
    
    float sDotN = max(dot(s, n), 0.0);
    vec3 diffuse = lights[index].Ld * Material.Kd * sDotN;
    
    vec3 spec = vec3(0.0);
    if(sDotN > 0.0) {
        float specFactor = 0.0;
        if(blinn) {
            vec3 halfwayDir = normalize(s + v);
            specFactor = pow(max(dot(n, halfwayDir), 0.0), Material.Shininess);
        } else {
            vec3 r = reflect(-s, n);
            specFactor = pow(max(dot(r, v), 0.0), Material.Shininess);
        }
        spec = lights[index].Ls * Material.Ks * specFactor;
    }
    return ambient + diffuse + spec;
}

void main() {
    vec3 n = normalize(Normal);
    vec3 v = normalize(-FragPos);
    
    vec3 result = vec3(0.0);
    
    for(int i = 0; i < numLights; ++i) {
        result += calculateLight(i, n, v);
    }
    
    FragColor = vec4(result, 1.0);
}