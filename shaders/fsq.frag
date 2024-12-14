#version 410 core  
out vec4 color; 

in vec2 vTexCoord;

uniform sampler2D uTexture;

float unpack(vec4 v) {
   return v.x + v.y / (256.0) + v.z / (256.0*256.0) + v.w / (256.0*256.0*256.0);
}

void main(void) 
{ 
	color = vec4(vec3(unpack(texture(uTexture,vTexCoord))), 1.0);
} 