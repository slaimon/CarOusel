#version 410 core 
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aColor; 
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec2 aTexCoord;

// transformed vertex attributes
out vec3 vPos;
out vec3 posWS;
out vec3 vNormal;
out vec2 vTexCoord;

// position of the lights in viewspace
#define NLAMPS 19
out vec3 vSunVS;
out vec3 vLampVS[NLAMPS];

// position of the sun in tangent space (for normal mapping)
out vec3 vSunTS;

// light coordinates (for shadow mapping)
out vec4 vPosLS;
out vec3 vSunWS;


// UNIFORMS:

// positions of the lights in worldspace
uniform vec3 uLamps[NLAMPS];
uniform vec3 uSunDirection;
uniform float uLampState;

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
   
   if (uLampState == 1.0) {
      for (int i = 0; i < NLAMPS; i++) {
         vLampVS[i] = (uView*vec4(uLamps[i],1.0)).xyz;
      }
   }
   
   if (uMode == 0)   // textured flat shading
      vTexCoord = aTexCoord;
   if (uMode == 1) { // normal map
      tangent = normalize(aTangent);
      bitangent = normalize(cross(aNormal,tangent));
      TF[0] = tangent;
      TF[1] = bitangent;
      TF[2] = normalize(aNormal);
      TF    = transpose(TF);

      vSunTS = TF * (inverse(uModel)*vec4(uSunDirection,0.0)).xyz;
      vTexCoord = aTexCoord;
   }
   
   // shadow mapping computations
   vPosLS = uSunMatrix * uModel * vec4(aPosition, 1.0);
   vSunWS = computeLightPosWS(vPosLS);

   // vertex computations
   vNormal = normalize(uView*uModel*vec4(aNormal, 0.0)).xyz;
   vec4 pws = (uModel*vec4(aPosition,1.0));
   posWS = pws.xyz;
   pws = uView*pws;
   vPos = pws.xyz; 
   gl_Position = uProj*pws;
}
