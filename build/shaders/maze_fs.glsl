#version 330 core
out vec4 FragColor;

struct Light {
    vec3 position;  // Camera position
    vec3 direction; // Camera direction
    float cutOff;   // Inner angle (strong light)
    float outerCutOff; // Outer angle (smooth edge)
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
	
    // Attenuation (light falls with distance)
    float constant;
    float linear;
    float quadratic;
};

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform vec3 viewPos;
uniform sampler2D texture_diffuse1;
uniform Light light;
uniform bool useTexture;
uniform vec3 objectColor;
uniform vec3 environmentTint; // Progressive color tint based on portal proximity

uniform float time;
uniform bool isPortal; // New uniform to toggle portal effect

void main()
{
    // --- PORTAL EFFECT (Liquid Silver / Magic Orb) ---
    if (isPortal) {
        // 1. Basic Setup
        vec3 norm = normalize(Normal);
        vec3 viewDir = normalize(viewPos - FragPos);
        
        // 2. Animated Pattern (Liquid Metal / Energy Plasma)
        // Combine sine waves at different frequencies
        float t = time * 2.0;
        float wave1 = sin(TexCoord.x * 20.0 + t);
        float wave2 = cos(TexCoord.y * 20.0 + t);
        float wave3 = sin((TexCoord.x + TexCoord.y) * 15.0 - t * 1.5);
        
        float pattern = (wave1 + wave2 + wave3); // Range approx -3 to 3
        pattern = smoothstep(0.0, 2.0, abs(pattern)); // Create sharp bright bands
        
        // 3. Colors
        vec3 silverColor = vec3(0.7, 0.75, 0.8); // Bluish Silver base
        vec3 brightSpot = vec3(1.0, 1.0, 1.0);   // Pure White highlights
        vec3 energyGlow = vec3(0.5, 0.8, 1.0);   // Cyan/Electric glow
        
        vec3 finalColor = mix(silverColor, brightSpot, pattern * 0.5);
        
        // 4. Fresnel Effect (Rim Lighting for Magic Orb look)
        // Edges glow brighter
        float fresnel = pow(1.0 - max(dot(viewDir, norm), 0.0), 3.0);
        finalColor += energyGlow * fresnel * 1.5;
        
        // 5. Pulsing Core
        float corePulse = sin(time * 3.0) * 0.1 + 0.9;
        finalColor *= corePulse;

        FragColor = vec4(finalColor, 1.0);
        return; 
    }

    // --- STANDARD LIGHTING (Existing Code) ---
    vec3 color = objectColor;
    if(useTexture) {
        color = texture(texture_diffuse1, TexCoord).rgb * objectColor;
    }
    
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    // --- FLASHLIGHT (SPOTLIGHT) CALCULATION ---
    vec3 lightDir = normalize(light.position - FragPos);
    
    // Theta: angle between light and fragment
    float theta = dot(lightDir, normalize(-light.direction)); 
    float epsilon = light.cutOff - light.outerCutOff;
    // Intensity: smooths the light edge
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    
    // --- AMBIENT ---
    vec3 ambient = light.ambient * color;
    
    // --- DIFFUSE ---
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * color;
    
    // --- SPECULAR ---
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
    vec3 specular = light.specular * spec;
    
    // --- ATTENUATION (Distance) ---
    float distance = length(light.position - FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    

    // Apply attenuation and flashlight intensity
    diffuse  *= attenuation * intensity;
    specular *= attenuation * intensity;
    ambient  *= attenuation;
    
    vec3 result = ambient + diffuse + specular;
    
    // Apply environment tint (progressive color transition to portal)
    result = result * environmentTint;

    // --- (OPTIONAL) FOG ---
    float fogDistance = length(viewPos - FragPos);
    float fogFactor = exp(-pow((fogDistance * 0.03), 2.0));
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    vec3 fogColor = vec3(0.05, 0.05, 0.05);
    
    result = mix(fogColor, result, fogFactor);

    FragColor = vec4(result, 1.0);
}
