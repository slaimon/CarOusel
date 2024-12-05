#version 410 core  
out vec4 color;

// light colors
#define AMBIENT_LIGHT      vec3(0.25,0.61,1.0) * 0.20
#define SUNLIGHT_COLOR     vec3(1.00,1.00,1.00)
#define LAMPLIGHT_COLOR    vec3(1.00,0.82,0.70)
#define HEADLIGHT_COLOR    vec3(1.00,1.00,0.00)

// lamp group parameters
#define NUM_LAMPS         19
#define NUM_ACTIVE_LAMPS   3

// positional lights attenuation coefficients
#define ATTENUATION_C1  1.0
#define ATTENUATION_C2  0.0
#define ATTENUATION_C3  0.005

// spotlight parameters
#define SPOTLIGHT_DIRECTION   vec3(0.0, -1.0, 0.0)

// headlights parameters
#define NUM_CARS     1
#define HEADLIGHT_SPREAD      0.1
#define HEADLIGHT_SPREAD_SQRT sqrt(HEADLIGHT_SPREAD)

// shadow mapping parameters
#define BIAS_PCF_SUN        0.005
#define BIAS_PCF_LAMP       0.001
#define BIAS_PCF_HEADLIGHT  0.05
#define BIAS_A       0.1
#define BIAS_MIN_E   0.001
#define BIAS_MAX_E   0.2


/*   ------   INPUTS   ------   */

// interpolated vertex attributes
in vec3 vPosWS;
in vec3 vNormalWS;
in vec2 vTexCoord;

// light coordinates
in vec3 vSunWS;
in vec4 vPosSunLS;
in vec4 vPosLampLS[NUM_LAMPS];
in vec4 vPosHeadlightLS[2*NUM_CARS];


/*   ------   UNIFORMS   ------   */

// coordinates of the lights in world space
uniform vec3 uSunDirection;
uniform float uSunState;
uniform vec3 uLamps[NUM_LAMPS];
uniform vec3 uLampDirection;
uniform vec3 uHeadlightPos[2*NUM_CARS];

// lamp parameters
uniform float uLampState;
uniform float uLampAngleIn;
uniform float uLampAngleOut;

// shadow maps
uniform float uDrawShadows;
uniform sampler2D uSunShadowmap;
uniform int uSunShadowmapSize;
uniform sampler2D uLampShadowmaps[NUM_LAMPS];
uniform int uLampShadowmapSize;
uniform sampler2D uHeadlightShadowmap;
uniform int uHeadlightShadowmapSize;

// rendering mode
uniform int uMode;

// material parameters
uniform vec3 uColor;
uniform float uShininess;
uniform float uDiffuse;
uniform float uSpecular;
uniform sampler2D uColorImage;

float unpack(vec4 v) {
   return v.x + v.y / (256.0) + v.z / (256.0*256.0) + v.w / (256.0*256.0*256.0);
}

float tanacos(float x) {
   return sqrt(1-x*x)/x;
}

vec4 sunlightColor() {
   return vec4(SUNLIGHT_COLOR, 1.0);//max(0.0, dot(uSunDirection, vec3(0.0,1.0,0.0))) * vec4(SUNLIGHT_COLOR,1.0);
}

// L and N must be normalized
float diffuseIntensity(vec3 L, vec3 N) {
   return uDiffuse * max(0.0, dot(L,N));
}

float specularIntensity(vec3 L, vec3 N, vec3 V) {
   float LN = diffuseIntensity(L,N);
   //vec3 R = -L+2*dot(L,N)*N;      // Phong
   vec3 H = normalize(L+V);         // Blinn-Phong
   
   return uSpecular * ((LN>0.f)?1.f:0.f) * max(0.0,pow(dot(V,H),uShininess));
}

float spotlightIntensity(vec3 lightPos, vec3 surfacePos) {
   vec3 lightDir = normalize(lightPos - surfacePos);   // the vector pointing from the fragment to the light source
   
   float cosine = dot(lightDir, -uLampDirection);
   if (cosine > uLampAngleIn)
      return 1.0;
   else if (cosine > uLampAngleOut) {
      float w = (cosine - uLampAngleOut)/(uLampAngleIn - uLampAngleOut);
      return w;
   }
   else
      return 0.0;
}

float headlightIntensity(int i) {
	if (vPosHeadlightLS[i].w < 0.0)
       return 0.0;
	vec2 texcoords = (vPosHeadlightLS[i]/vPosHeadlightLS[i].w).xy;
	float d = length(texcoords);
    if (d > 1.0)
      return 0.0;
	if (d <= HEADLIGHT_SPREAD_SQRT)
	   return 1.0;
	else
	   return HEADLIGHT_SPREAD / (d*d) - HEADLIGHT_SPREAD;
}

float attenuation(vec3 lightPos, vec3 surfacePos) {
   float d = length(lightPos-surfacePos);
   
   return min(1.0, 1.0/(ATTENUATION_C1+d*ATTENUATION_C2+d*d*ATTENUATION_C3));
}

float attenuation(float d) {
   return min(1.0, 1.0/(ATTENUATION_C1+d*ATTENUATION_C2+d*d*ATTENUATION_C3));
}

float isLit(vec3 L, vec3 N, vec4 posLS, sampler2D shadowmap) {
   if (uDrawShadows == 0.0)
      return 1.0;

   float bias = clamp(BIAS_A*tanacos(dot(N,L)), BIAS_MIN_E, BIAS_MAX_E);
   vec4 pLS = (posLS/posLS.w)*0.5+0.5;
   if (pLS.x < 0.0 || pLS.x > 1.0 || pLS.y < 0.0 || pLS.y > 1.0)
      return 0.0;
   float depth = unpack(texture(shadowmap,pLS.xy));
   
   return ((depth + bias < pLS.z) ? (0.0) : (1.0));
}

float isLitPCF(vec3 L, vec3 N, vec4 posLS, sampler2D shadowmap, int shadowmapSize, float bias) {
   if (uDrawShadows == 0.0)
      return 1.0;
   vec4 pLS = (posLS/posLS.w)*0.5+0.5;
   if (pLS.x < 0.0 || pLS.x > 1.0 || pLS.y < 0.0 || pLS.y > 1.0)
      return 0.0;

   float storedDepth;
   float lit = 1.0;
   
   for(float x = 0.0; x < 5.0; x+=1.0) {
      for(float y = 0.0; y < 5.0; y+=1.0) {
         storedDepth =  unpack(texture(shadowmap, pLS.xy + vec2(-2.0+x,-2.0+y)/shadowmapSize));
         if(storedDepth + bias < pLS.z )    
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
      surfaceNormal = normalize(cross(dFdx(vPosWS),dFdy(vPosWS)));
      sunIntensityDiff = diffuseIntensity(vSunWS, surfaceNormal);
      diffuseColor = texture2D(uColorImage,vTexCoord.xy);
   }
   // monochrome flat shading
   if (uMode == 1) {
      surfaceNormal = normalize(cross(dFdx(vPosWS),dFdy(vPosWS)));
      sunIntensityDiff = diffuseIntensity(uSunDirection, surfaceNormal);
      diffuseColor = vec4(uColor, 1.0);
   }
   // textured phong shading
   if (uMode == 2) {
      surfaceNormal = vNormalWS;
      sunIntensityDiff = diffuseIntensity(vSunWS, surfaceNormal);
      sunIntensitySpec = specularIntensity(vSunWS, surfaceNormal, normalize(-vPosWS));
      diffuseColor = texture2D(uColorImage,vTexCoord.xy);
   }
   // monochrome phong shading
   if (uMode == 3) {
      surfaceNormal = vNormalWS;
      sunIntensityDiff = diffuseIntensity(vSunWS, surfaceNormal);
      sunIntensitySpec = specularIntensity(vSunWS, surfaceNormal, normalize(-vPosWS));
      diffuseColor = vec4(uColor,1.0);
   }
   
   float spotint, lit = 1.0;
   uint j;
   if (uLampState == 1.0) {
      for (int i = 0; i < NUM_ACTIVE_LAMPS; ++i) {
	     // if the fragment is outside this lamp's light cone, skip all calculations
	     spotint = spotlightIntensity(uLamps[i], vPosWS);
		 if (spotint == 0.0)
		    continue;

         lampsIntensity += spotint * isLitPCF(normalize(uLamps[i] - vPosWS), surfaceNormal, vPosLampLS[i], uLampShadowmaps[i], uLampShadowmapSize, BIAS_PCF_LAMP);
      }
      lampsContrib = lampsIntensity * vec4(LAMPLIGHT_COLOR, 1.0);
   }
   
   if (uSunState == 1.0) {
      sunContrib = sunlightColor() * (sunIntensityDiff + sunIntensitySpec) *
	               isLitPCF(vSunWS, surfaceNormal, vPosSunLS, uSunShadowmap, uSunShadowmapSize, BIAS_PCF_SUN);
   }
   
   float headlightintensity = 0.0;
   for (int i = 0; i < 2*NUM_CARS; ++i) {
      float headint = headlightIntensity(i);
	  // if the fragment is outside this headlight's light cone, skip all calculations
	  if (headint == 0.0)
	     continue;
      headlightintensity += attenuation(vPosHeadlightLS[i].w) * headint * 
	                        isLitPCF(normalize(uHeadlightPos[i]), surfaceNormal, vPosHeadlightLS[i], uHeadlightShadowmap[i], uHeadlightShadowmapSize, BIAS_PCF_HEADLIGHT);
   }
   vec4 headlightContrib = vec4(HEADLIGHT_COLOR,1.0) * headlightintensity;
   
   color = diffuseColor * clamp(vec4(AMBIENT_LIGHT,1.0) + (lampsContrib + sunContrib + headlightContrib), 0.0, 1.0);
} 
