#version 330 core
layout(location=0) in vec4 in_Position;
uniform mat4 model;
uniform mat4 lightSpaceMatrix;
void main(){ gl_Position = lightSpaceMatrix * model * in_Position; }
