#version 410 core 
layout (location = 0) in vec3 aPosition; 

uniform mat4 uModel;
uniform mat4 uLightMatrix;

// unused
uniform int uMode;
uniform vec3 uColor;
uniform sampler2D uColorImage;
uniform sampler2D uNormalmapImage;
uniform float uShininess;
uniform float uDiffuse;
uniform float uSpecular;

void main(void) 
{ 
    gl_Position = uLightMatrix*uModel*vec4(aPosition, 1.0); 
}
