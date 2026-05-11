#version 460 core

uniform float iTime; // input from CPU C++ App = time in seconds   
uniform float object_alpha; // <--- PŘIDÁNO: Tudy sem přiteče hodnota z C++

out vec4 FragColor; // output color of current fragment: MUST be written

void main() {
    const float pi    = 3.1415f;
    const float pi3   = pi/3.0f;
    const float d_pi3 = pi3*2.0f;
  
    float r = abs(sin(        gl_FragCoord.y * 0.05f + 3.0f * iTime));
    float g = abs(sin(pi3   + gl_FragCoord.y * 0.05f + 3.0f * iTime));
    float b = abs(sin(d_pi3 + gl_FragCoord.y * 0.05f + 3.0f * iTime));

    FragColor = vec4(r, g, b, object_alpha);
}