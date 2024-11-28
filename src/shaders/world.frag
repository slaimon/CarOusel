#version 410 core  
out vec4 color; 

in vec3 vPosVS;
in vec3 vPosWS;
in vec3 vNormalWS;
in vec2 vTexCoord;

#define AMBIENT_LIGHT      vec3(0.25,0.61,1.0) * 0.20
#define SUNLIGHT_COLOR     vec3(1.00,1.00,1.00)
#define LAMPLIGHT_COLOR    vec3(1.00,0.82,0.70)
#define SHININESS          1.5
#define SPECULAR_WEIGHT    0.0

// positional lights attenuation coefficients
#define ATTENUATION_C1  0.01
#define ATTENUATION_C2  1.0
#define ATTENUATION_C3  20.0

// spotlight parameters
#define SPOTLIGHT_DIRECTION   vec3(0.0, -1.0, 0.0)

// positions of the lights in viewspace
#define NLAMPS     19
#define NLAMPS_ON  1
in vec3 vLampVS[NLAMPS];
in vec3 vSunVS;

// Normal Mapping
in vec3 vSunTS;

// Shadow Mapping
#define BIAS_PCF     0.005
#define BIAS_A       0.01
#define BIAS_MIN_E   0.001
#define BIAS_MAX_E   0.2
in vec4 vPosSunLS;
in vec3 vSunWS;
in vec4 vPosLampLS[NLAMPS];


// UNIFORMS:

uniform vec3 uSunDirection;
uniform float uSunState;

uniform vec3 uLamps[NLAMPS];
uniform float uLampState;
uniform float uLampAngleIn;
uniform float uLampAngleOut;
uniform vec3 uLampDirection;

uniform float uDrawShadows;
uniform int uSunShadowmapSize;
uniform sampler2D uSunShadowmap;
uniform sampler2D uLampShadowmaps[NLAMPS];
uniform int uLampShadowmapSize;

uniform int uMode;
uniform vec3 uColor;
uniform sampler2D uColorImage;
uniform sampler2D uNormalmapImage;

float tanacos(float x) {
   return sqrt(1-x*x)/x;
}

vec4 sunlightColor() {
   return vec4(SUNLIGHT_COLOR, 1.0);//max(0.0, dot(uSunDirection, vec3(0.0,1.0,0.0))) * vec4(SUNLIGHT_COLOR,1.0);
}

// L and N must be normalized
float lightIntensity(vec3 L, vec3 N) {
   return max(0.0, dot(L,N));
}

float specularIntensity(vec3 L, vec3 N, vec3 V) {
   float LN = lightIntensity(L,N);
   //vec3 R = -L+2*dot(L,N)*N;      // Phong
   vec3 H = normalize(L+V);         // Blinn-Phong
   
   return SPECULAR_WEIGHT * ((LN>0.f)?1.f:0.f) * max(0.0,pow(dot(V,H),SHININESS));
}

float spotlightIntensity(vec3 lightPos, vec3 surfacePos) {
   vec3 lightDir = normalize(lightPos - surfacePos);   // the vector pointing from the fragment to the light source
   
   float cosine = dot(lightDir, -SPOTLIGHT_DIRECTION);
   if (cosine > uLampAngleIn)
      return 1.0;
   else if (cosine > uLampAngleOut) {
      float w = (cosine - uLampAngleOut)/(uLampAngleIn - uLampAngleOut);
      return w;
   }
   else
      return 0.0;
}

float attenuation(vec3 lightPos, vec3 surfacePos) {
   float d = length(lightPos-surfacePos);
   
   return min(1.0, 1.0/(ATTENUATION_C1+d*ATTENUATION_C2+d*d*ATTENUATION_C3));
}

float isLit(vec3 L, vec3 N, vec4 posLS, sampler2D shadowmap) {
   if (uDrawShadows == 0.0)
      return 1.0;

   float bias = clamp(BIAS_A*tanacos(dot(N,L)), BIAS_MIN_E, BIAS_MAX_E);
   vec4 pLS = (posLS/posLS.w)*0.5+0.5;
   if (pLS.y < 0.0 || pLS.x > 1.0 || pLS.y < 0.0 || pLS.y > 1.0)
      return 0.0;
   float depth = texture(shadowmap,pLS.xy).x;
   
   return ((depth + bias < pLS.z) ? (0.0) : (1.0));
}

float isLitPCF(vec3 L, vec3 N, vec4 posLS, sampler2D shadowmap, int shadowmapSize) {
   if (uDrawShadows == 0.0)
      return 1.0;
   // the pixel is in shadow if the normal points away from the light source
   if (dot(normalize(L), normalize(N)) < 0.0)
      return 0.0;

   float storedDepth;
   float lit = 1.0;
   vec4 pLS = (posLS/posLS.w)*0.5+0.5;
   
   for(float x = 0.0; x < 5.0; x+=1.0) {
      for(float y = 0.0; y < 5.0; y+=1.0) {
         storedDepth =  texture(shadowmap, pLS.xy + vec2(-2.0+x,-2.0+y)/shadowmapSize).x;
         if(storedDepth + BIAS_PCF < pLS.z )    
            lit  -= 1.0/25.0;
      }
   }
   
   return lit;
}

void main(void) { 
   vec3 surfaceNormal;
   vec4 diffuseColor;
   float sunIntensityDiff;
   float sunIntensitySpec = 0.0;
   float lampsIntensity = 0.0;
   vec4 lampsContrib;
   vec4 sunContrib;
   
   // textured flat shading
   if (uMode == 0) {
      surfaceNormal = normalize(cross(dFdx(vPosVS),dFdy(vPosVS)));
      sunIntensityDiff = lightIntensity(vSunVS, surfaceNormal);
      diffuseColor = texture2D(uColorImage,vTexCoord.xy);
   }
   // monochrome flat shading
   if (uMode == 1) {
      surfaceNormal = normalize(cross(dFdx(vPosVS),dFdy(vPosVS)));
      sunIntensityDiff = lightIntensity(vSunVS, surfaceNormal);
      diffuseColor = vec4(uColor, 1.0);
   }
   // textured phong shading
   if (uMode == 2) {
      surfaceNormal = vNormalWS;
      sunIntensityDiff = lightIntensity(vSunWS, surfaceNormal);
      sunIntensitySpec = specularIntensity(vSunWS, surfaceNormal, normalize(-vPosWS));
      diffuseColor = texture2D(uColorImage,vTexCoord.xy);
   }
   // monochrome phong shading
   if (uMode == 3) {
      surfaceNormal = vNormalWS;
      sunIntensityDiff = lightIntensity(vSunWS, surfaceNormal);
      sunIntensitySpec = specularIntensity(vSunWS, surfaceNormal, normalize(-vPosWS));
      diffuseColor = vec4(uColor,1.0);
   }
   
   float spotint, lit = 1.0;
   if (uLampState == 1.0) {
      for (int i=0; i<NLAMPS_ON; i++) {
	     // if the fragment is outside this lamp's light cone, skip all calculations
	     spotint = spotlightIntensity(uLamps[i], vPosWS);
		 if (spotint == 0.0)
		    continue;

         lampsIntensity += isLit(normalize(uLamps[i] - vPosWS), surfaceNormal, vPosLampLS[i], uLampShadowmaps[i]);
      }
      lampsContrib = lampsIntensity * vec4(LAMPLIGHT_COLOR, 1.0);
   }
   
   if (uSunState == 1.0) {
      sunContrib = sunlightColor() * (sunIntensityDiff + sunIntensitySpec) *
	               isLitPCF(vSunWS, surfaceNormal, vPosSunLS, uSunShadowmap, uSunShadowmapSize);
   }
   
   color = diffuseColor * clamp(vec4(AMBIENT_LIGHT,1.0) + (lampsContrib + sunContrib), 0.0, 1.0);
} 
