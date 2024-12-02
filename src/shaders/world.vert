#version 410 core 
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aColor; 
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec2 aTexCoord;

// transformed vertex attributes
out vec3 vPosVS;
out vec3 vPosWS;
out vec3 vNormalWS;
out vec2 vTexCoord;

// position of the lights in viewspace
#define NUM_LAMPS         19
#define NUM_ACTIVE_LAMPS   3
out vec3 vSunVS;
out vec3 vLampVS[NUM_LAMPS];

// position of the sun in tangent space (for normal mapping)
out vec3 vSunTS;

// light coordinates (for shadow mapping)
out vec4 vPosSunLS;
out vec3 vSunWS;
out vec4 vPosLampLS[NUM_LAMPS];


// UNIFORMS:

// positions of the lights in worldspace
uniform vec3 uLamps[NUM_LAMPS];
uniform vec3 uSunDirection;
uniform float uLampState;
uniform uint uNumActiveLamps;
uniform uint uActiveLamps[NUM_LAMPS];

uniform mat4 uLampMatrix[NUM_LAMPS];
uniform mat4 uSunMatrix;
uniform int uMode;

uniform mat4 uProj;
uniform mat4 uView;
uniform mat4 uModel;


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
   vec3 tangent, bitangent;
   vec3 ViewVS;
   mat3 TF;
   
   vSunVS = normalize((uView*vec4(uSunDirection,0.0)).xyz);
   
   uint j;
   if (uLampState == 1.0) {
      for (int i = 0; i < NUM_ACTIVE_LAMPS; i++) {
         vLampVS[i] = (uView*vec4(uLamps[i],1.0)).xyz;
		 vPosLampLS[i] = (uLampMatrix[i]*uModel*vec4(aPosition, 1.0));
      }
   }
   
   if (uMode == 0)   // textured flat shading
      vTexCoord = aTexCoord;
   
   // shadow mapping computations
   vPosSunLS = uSunMatrix * uModel * vec4(aPosition, 1.0);
   vSunWS = computeLightPosWS(vPosSunLS);

   // vertex computations
   vNormalWS = normalize(uModel*vec4(aNormal, 0.0)).xyz;
   vec4 pws = (uModel*vec4(aPosition,1.0));
   vPosWS = pws.xyz;
   pws = uView*pws;
   vPosVS = pws.xyz; 
   gl_Position = uProj*pws;
}
