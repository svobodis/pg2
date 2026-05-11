#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 uM_m = mat4(1.0);
uniform mat4 uV_m = mat4(1.0);
uniform mat4 uP_m = mat4(1.0);

out VS_OUT {
    vec3 FragPos; // Pozice vrcholu vůči kameře
    vec3 Normal;  // Natočení plochy
    vec2 TexCoord;
} vs_out;

void main(void) {
    mat4 mv_m = uV_m * uM_m;
    vec4 viewPos = mv_m * vec4(aPos, 1.0);
    
    vs_out.FragPos = viewPos.xyz;
    vs_out.Normal = mat3(mv_m) * aNormal;
    vs_out.TexCoord = aTexCoord;
    
    gl_Position = uP_m * viewPos;
}