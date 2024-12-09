#version 410 core 
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aColor; 
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec2 aTexCoord;

// lamp group parameters
#define NUM_LAMPS         19
#define NUM_ACTIVE_LAMPS   3

// headlights parameters
#define NUM_CARS     1


/*   ------   OUTPUTS   ------   */

// transformed vertex attributes
out vec3 vPosWS;
out vec3 vNormalWS;
out vec2 vTexCoord;

// light coordinates
out vec3 vSunWS;
out vec4 vPosSunLS;
out vec4 vPosLampLS[NUM_LAMPS];
out vec4 vPosHeadlightLS[2*NUM_CARS];


/*   ------   UNIFORMS   ------   */

uniform float uDrawShadows;

// coordinates of the lights in worldspace
uniform vec3 uLamps[NUM_LAMPS];
uniform vec3 uSunDirection;

// lights' activation state
uniform float uSunState;
uniform float uLampState;
uniform float uHeadlightState;

// light matrices
uniform mat4 uLampMatrix[NUM_LAMPS];
uniform mat4 uSunMatrix;
uniform mat4 uHeadlightMatrix[2*NUM_CARS];

// render mode
uniform int uMode;

// transformation matrices
uniform mat4 uView;
uniform mat4 uModel;
uniform mat4 uProj;


vec3 computeLightPosWS (vec4 vPosLS) {
   mat4 invSunMatrix = inverse(uSunMatrix);
   
   vec4 vPosLS_NDC = (vPosLS/vPosLS.w);
   vec4 far = vec4(vPosLS_NDC.xy, 1,1.0);
   vec4 near = vec4(vPosLS_NDC.xy,-1,1.0);

   vec4 farWS = invSunMatrix * far;
   farWS /= farWS.w;

   vec4 nearWS = invSunMatrix * near;
   nearWS /= nearWS.w;
   
   return normalize((nearWS-farWS).xyz);
}


void main(void) {
   
   // textured flat shading
   if (uMode == 0)
      vTexCoord = aTexCoord;
   
   // sun position in light-space and world-space
   if (uSunState == 1.0) {
      vPosSunLS = uSunMatrix * uModel * vec4(aPosition, 1.0);
      vSunWS = computeLightPosWS(vPosSunLS);
   }
   
   // shadow mapping for lamps
   if (uDrawShadows == 1.0 && uLampState == 1.0) {
      for (int i = 0; i < NUM_ACTIVE_LAMPS; i++) {
         vPosLampLS[i] = uLampMatrix[i] * uModel * vec4(aPosition, 1.0);
      }
   }
   
   // projective texturing for headlights
   if (uHeadlightState == 1.0) {
      for (int i = 0; i < 2*NUM_CARS; ++i) {
         vPosHeadlightLS[i] = uHeadlightMatrix[i] * uModel * vec4(aPosition, 1.0);
      }
   }

   // vertex computations
   vNormalWS = normalize(uModel*vec4(aNormal, 0.0)).xyz;
   vec4 pws = (uModel*vec4(aPosition,1.0));
   vPosWS = pws.xyz;
   gl_Position = uProj*uView*pws;
}
