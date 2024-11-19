#version 410 core  
out vec4 color; 

in vec3 vPos;
in vec3 vNormal;
in vec2 vTexCoord;

#define AMBIENT_LIGHT      vec3(0.1,0.1,0.1)
#define SUNLIGHT_COLOR     vec3(1.0,1.0,1.0)
#define LAMPLIGHT_COLOR    vec3(1.0,0.9,0.8)
#define SHININESS          2.0

// positional lights attenuation coefficients
#define ATTENUATION_C1  0.5
#define ATTENUATION_C2  0.0
#define ATTENUATION_C3  10.0

// spotlight parameters
#define LAMP_ANGLE_IN   radians(45.0)
#define LAMP_ANGLE_OUT  radians(90.0)

// positions of the lights in viewspace
#define NLAMPS 19
in vec3 vLampVS[NLAMPS];
in vec3 vSunVS;

// Normal Mapping
in vec3 vSunTS;

// Shadow Mapping
#define BIAS_PCF     0.005
#define BIAS_A       0.01
#define BIAS_MIN_E   0.001
#define BIAS_MAX_E   0.2
in vec4 vPosLS;
in vec3 vSunWS;


// UNIFORMS:

// positions of the lights in worldspace
uniform vec3 uLamps[NLAMPS];
uniform vec3 uSun;
uniform float uLampState;
uniform float uDrawShadows;

uniform vec3 uColor;
uniform int uMode;
uniform sampler2D uColorImage;
uniform sampler2D uNormalmapImage;
uniform sampler2D uShadowMap;
uniform int uShadowMapSize;


float sunIntensity() {
   return max(0.0, dot(uSun, vec3(0.0,1.0,0.0)));
}

// L and N must be normalized
float lightIntensity(vec3 L, vec3 N) {
   return max(0.0, dot(L,N));
}

float specularIntensity(vec3 L, vec3 N, vec3 V) {
   float LN = lightIntensity(L,N);
   vec3 R = -L+2*dot(L,N)*N;
   
   return ((LN>0.f)?1.f:0.f) * max(0.0,pow(dot(V,R),SHININESS));
}
/*
float spotlightIntensity(vec3 lightPos, vec3 surfacePos) {
   float angle = acos(dot(normalize(lightPos-surfacePos),vec3(0.,-1.0,0.)));
   
   if (angle < LAMP_ANGLE_IN)
      return 1.0;
   if (LAMP_ANGLE_IN < angle && angle < LAMP_ANGLE_OUT)
      return cos(angle-LAMP_ANGLE_IN);
   if (LAMP_ANGLE_OUT < angle)
      return 0.0;
}

float attenuation(vec3 lightPos, vec3 surfacePos) {
   float d = length(lightPos-surfacePos);
   return min(1.0, 1.0/(ATTENUATION_C1+d*ATTENUATION_C2+d*d*ATTENUATION_C3));
}
*/
float isLit(vec3 L, vec3 N) {
   if (uDrawShadows == 0.0)
      return 1.0;

   float bias = clamp(BIAS_A*tan(acos(dot(N,L))), BIAS_MIN_E, BIAS_MAX_E);
   vec4 pLS = (vPosLS/vPosLS.w)*0.5+0.5;
   float depth = texture(uShadowMap,pLS.xy).x;
   
   return ((depth + bias < pLS.z) ? (0.0) : (1.0));
}

float isLitPCF(vec3 L, vec3 N) {
   if (uDrawShadows == 0.0)
      return 1.0;

   float storedDepth;
   float lit = 1.0;
   vec4 pLS = (vPosLS/vPosLS.w)*0.5+0.5;
   
   for(float x = 0.0; x < 5.0; x+=1.0) {
      for(float y = 0.0; y < 5.0; y+=1.0) {
         storedDepth =  texture(uShadowMap, pLS.xy + vec2(-2.0+x,-2.0+y)/uShadowMapSize).x;
         if(storedDepth + BIAS_PCF < pLS.z )    
            lit  -= 1.0/25.0;
      }
   }
   
   return lit;
}

// this produce the Hue for v:0..1 (for debug purposes)
vec3 hsv2rgb(float  v)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(vec3(v,v,v) + K.xyz) * 6.0 - K.www);
    return   mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0),1.0);
}

void main(void) { 
   vec3 surfaceNormal;
   vec4 diffuseColor;
   float sunIntensitySpec = 0.0;
   float sunIntensityDiff;
   vec4 lampsContrib;
   
   vec3 sunToSurfaceVS = normalize(vSunVS-vPos);
   vec3 lampToSurfaceVS;
   
   // textured flat shading
   if (uMode == 0) {
      surfaceNormal = normalize(cross(dFdx(vPos),dFdy(vPos)));
      sunIntensityDiff = lightIntensity(sunToSurfaceVS, surfaceNormal);
      diffuseColor = texture2D(uColorImage,vTexCoord.xy);
   }
   // normal map shading
   if (uMode == 1) {
      surfaceNormal = texture2D(uNormalmapImage,vTexCoord.xy).xyz ;
      surfaceNormal = normalize(surfaceNormal*2.0-1.0);
      sunIntensityDiff = lightIntensity(normalize(vSunTS), surfaceNormal);
      diffuseColor = texture2D(uColorImage,vTexCoord.xy);
   }
   // monochrome flat shading
   if (uMode == 2) {
      surfaceNormal = normalize(cross(dFdx(vPos),dFdy(vPos)));
      sunIntensityDiff = lightIntensity(sunToSurfaceVS, surfaceNormal);
      //sunIntensitySpec = specularIntensity(normalize(sunToSurfaceVS), normalize(surfaceNormal), normalize(-vPos));
      diffuseColor = vec4(uColor, 1.0);
   }
   // textured phong shading
   if (uMode == 3) {
      surfaceNormal = normalize(vNormal);
      sunIntensityDiff = lightIntensity(sunToSurfaceVS, surfaceNormal);
      sunIntensitySpec = specularIntensity(sunToSurfaceVS, surfaceNormal, normalize(-vPos));
      diffuseColor = texture2D(uColorImage,vTexCoord.xy);
   }
   // monochrome phong shading
   if (uMode == 4) {
      surfaceNormal = normalize(vNormal);
      sunIntensityDiff = lightIntensity(sunToSurfaceVS, surfaceNormal);
      sunIntensitySpec = specularIntensity(sunToSurfaceVS, surfaceNormal, normalize(-vPos));
      diffuseColor = vec4(uColor,1.0);
   }

   /*
   for (int i=0; i<NLAMPS; i++) {
      lampToSurfaceVS = normalize(vLampVS[i]-vPos);
      lampsContrib += spotlightIntensity(vLampVS[i], vPos) * lightIntensity(lampToSurfaceVS, surfaceNormal);
   }
   lampsContrib *= vec4(LAMPLIGHT_COLOR, 1.0)/NLAMPS;
   */
   
   vec4 sunContrib = sunIntensityDiff * vec4(SUNLIGHT_COLOR,1.0) + sunIntensitySpec * vec4(SUNLIGHT_COLOR,1.0);
   
   color = diffuseColor *
            (vec4(AMBIENT_LIGHT,1.0) +
            sunIntensity() * sunContrib * isLitPCF(sunToSurfaceVS, surfaceNormal));
} 
