#version 410 core  
out vec4 color;

uniform int uMode;

vec4 pack(const float d) {
   if (d == 1.0)
      return vec4(1.0);
   const vec4 bitshift = vec4(1.0, 256.0, 256.0*256.0, 256.0*256.0*256.0);
   const vec4 bitmask = vec4(1.0/256.0, 1.0/256.0, 1.0/256.0, 0.0);
   
   vec4 result = fract(d * bitshift);
        result -= result.yzwx * bitmask;
   
   return result;
}

void main(void) 
{ 
 	color = pack(gl_FragCoord.z);
} 
