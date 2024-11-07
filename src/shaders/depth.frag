#version 430 core  
out vec4 color;

uniform int uMode;

void main(void) 
{ 
 	color = vec4(vec3(gl_FragCoord.z),1.0);
} 
