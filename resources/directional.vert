#version 460 core


layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;


uniform mat4 uM_m = mat4(1.0);
uniform mat4 uV_m = mat4(1.0);
uniform mat4 uP_m = mat4(1.0);


uniform vec3 light_position = vec3(1.0, 1.0, 1.0);

// Výstupní balíček pro Fragment Shader
out VS_OUT {
    vec3 N;
    vec3 L;
    vec3 V;
    vec2 texCoord;
} vs_out;

void main(void) {
    // Sloučení Model a View matice
    mat4 mv_m = uV_m * uM_m;

    // Přepočet pozice vrcholu do View Space (prostoru kamery)
    // Protože máme aPos jako vec3, musíme z něj udělat vec4
    vec4 P = mv_m * vec4(aPos, 1.0);

    // Výpočet normály (směrování plochy) v prostoru kamery
    vs_out.N = mat3(mv_m) * aNormal;
    
    // Světlo (Zatím natvrdo předané z uniformu)
    vs_out.L = light_position;
    
    // Pohledový vektor (Od bodu do kamery, kamera je v bodě 0,0,0)
    vs_out.V = -P.xyz;

    // Předání UV souřadnic
    vs_out.texCoord = aTexCoord;

    // Finální pozice na monitoru
    gl_Position = uP_m * P;
}