#version 460 core


layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

// Naše matice
uniform mat4 uM_m = mat4(1.0);
uniform mat4 uV_m = mat4(1.0);
uniform mat4 uP_m = mat4(1.0);


uniform vec3 light_position = vec3(0.0, 0.0, 0.0);

// Výstupy pro Fragment Shader
out VS_OUT {
    vec3 N;
    vec3 L;
    vec3 V;
    vec2 texCoord;
} vs_out;

void main(void) {
    // Sloučení Model a View matice
    mat4 mv_m = uV_m * uM_m;

    // Přepočet pozice vrcholu do View Space
    vec4 P = mv_m * vec4(aPos, 1.0);

    // Výpočet normály (natočení plochy) v prostoru kamery
    vs_out.N = mat3(mv_m) * aNormal;
    
    // VÝPOČET BODOVÉHO SVĚTLA
    // L = pozice_světla - pozice_vrcholu
    vs_out.L = light_position - P.xyz;
    
    // Pohledový vektor (od bodu do kamery)
    vs_out.V = -P.xyz;

    // Předání UV souřadnic pro texturu
    vs_out.texCoord = aTexCoord;

    // Finální umístění na obrazovce
    gl_Position = uP_m * P;
}