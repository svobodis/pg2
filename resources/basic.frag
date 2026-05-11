#version 460 core

//in vec3 color; // input from vertex stage of graphics pipeline, automatically interpolated
out vec4 FragColor; // output color of current fragment: MUST be written

void main()
{
    FragColor = vec4(1.0, 0.5, 0.2, 1.0); // copy RGB color, add Alpha=1.0 (not transparent)
}
