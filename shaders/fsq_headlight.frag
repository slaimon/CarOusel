#version 430 core

#define HEADLIGHT_SPREAD      0.1
#define HEADLIGHT_SPREAD_SQRT sqrt(HEADLIGHT_SPREAD)

out vec4 color;

in vec2 vTexCoord;

void main(void) {
   float r;
   float d = length(vTexCoord);
   if (d > 1.0)
      r = 0.0;
   if (d <= HEADLIGHT_SPREAD_SQRT)
      r = 1.0;
   else
      r = HEADLIGHT_SPREAD / (d*d) - HEADLIGHT_SPREAD;

   color = vec4(vec3(r), 1.0);
} 