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
out vec3 vPosVS;
out vec3 vNormalWS;
out vec3 vNormalVS;
out vec2 vTexCoord;

// light coordinates
out vec3 vSunVS;
out vec3 vLampVS[NUM_ACTIVE_LAMPS];
out vec3 vHeadlightVS[2*NUM_CARS];
out vec4 vPosSunLS;
out vec4 vPosLampLS[NUM_ACTIVE_LAMPS];
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


void main(void) {
   vec4 pws = uModel * vec4(aPosition, 1.0);
   
   // textured flat shading
   if (uMode == 0)
      vTexCoord = aTexCoord;
   
   // sun position in light-space
   vSunVS = (uView * vec4(uSunDirection, 1.0)).xyz;
   if (uDrawShadows == 1.0 && uSunState == 1.0) {
      vPosSunLS = uSunMatrix * pws;
   }
   
   // shadow mapping for lamps
   if (uLampState == 1.0) {
      for (int i = 0; i < NUM_ACTIVE_LAMPS; i++) {
	     vLampVS[i] = (uView * vec4(uLamps[i], 1.0)).xyz;
	  }
      if (uDrawShadows == 1.0) {
         for (int i = 0; i < NUM_ACTIVE_LAMPS; i++) {
            vPosLampLS[i] = uLampMatrix[i] * pws;
         }
      }
   }
   
   // projective texturing for headlights
   if (uHeadlightState == 1.0) {
      for (int i = 0; i < 2*NUM_CARS; ++i) {
	     vHeadlightVS[i] = (uView * uHeadlightMatrix[i][3]).xyz;
         vPosHeadlightLS[i] = uHeadlightMatrix[i] * pws;
      }
   }

   // vertex computations
   vec4 vws = uModel * vec4(aNormal, 0.0);
   vNormalWS = normalize(vws).xyz;
   vNormalVS = normalize(uView * vws).xyz;
   vPosWS = pws.xyz;
   vPosVS = (uView * pws).xyz;
   gl_Position = uProj*uView*pws;
}
