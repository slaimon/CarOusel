#version 410 core  
out vec4 color;

// light colors
#define AMBIENT_LIGHT      vec3(0.25,0.61,1.0) * 0.35    // clear blue
#define SUNLIGHT_COLOR     vec3(1.00,1.00,1.00)          // pure white
#define LAMPLIGHT_COLOR    vec3(1.00,0.82,0.70)          // sodium vapor
#define HEADLIGHT_COLOR    vec3(1.00,0.73,0.00)          // selective yellow
//#define HEADLIGHT_COLOR    vec3(1.00,0.84,0.67)          // tungsten light

// lamp group parameters
#define NUM_LAMPS         19
#define NUM_ACTIVE_LAMPS   3

// positional lights attenuation coefficients
#define ATTENUATION_C1  1.0
#define ATTENUATION_C2  0.0
#define ATTENUATION_C3  0.001

// spotlight parameters
#define SPOTLIGHT_DIRECTION   vec3(0.0, -1.0, 0.0)

// headlights parameters
#define NUM_CARS     1
#define HEADLIGHT_SPREAD      0.1
#define HEADLIGHT_SPREAD_SQRT sqrt(HEADLIGHT_SPREAD)

// shadow mapping parameters
#define BIAS_PCF_SUN        0.005
#define BIAS_PCF_LAMP       0.001
#define BIAS_A       0.002
#define BIAS_MIN_E   0.0001
#define BIAS_MAX_E   0.01


/*   ------   INPUTS   ------   */

// interpolated vertex attributes
in vec3 vPosWS;
in vec3 vPosVS;
in vec3 vNormalWS;
in vec3 vNormalVS;
in vec2 vTexCoord;

// light coordinates
in vec3 vSunVS;
in vec3 vLampVS[NUM_ACTIVE_LAMPS];
in vec3 vHeadlightVS[2*NUM_CARS];
in vec4 vPosSunLS;
in vec4 vPosLampLS[NUM_ACTIVE_LAMPS];
in vec4 vPosHeadlightLS[2*NUM_CARS];


/*   ------   UNIFORMS   ------   */

// coordinates of the lights in world space
uniform vec3 uSunDirection;
uniform vec3 uLamps[NUM_LAMPS];
uniform vec3 uLampDirection;
uniform vec3 uHeadlightPos[2*NUM_CARS];

// lights' activation state
uniform float uSunState;
uniform float uLampState;
uniform float uHeadlightState;

// lamp parameters
uniform float uLampAngleIn;
uniform float uLampAngleOut;

// shadow maps
uniform float uDrawShadows;
uniform sampler2D uSunShadowmap;
uniform int uSunShadowmapSize;
uniform sampler2D uLampShadowmaps[NUM_LAMPS];
uniform int uLampShadowmapSize;
uniform sampler2D uHeadlightShadowmap[2*NUM_CARS];
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

float sunlightIntensity() {
   float c = dot(uSunDirection, vec3(0.0,1.0,0.0));
   return uSunState * sqrt(max(0.0, c));
}

// L and N must be normalized
float diffuseIntensity(vec3 L, vec3 N) {
   return uDiffuse * max(0.0, dot(L,N));
}

float specularIntensity(vec3 L, vec3 N, vec3 V) {
   vec3 H = normalize(L+V);
   float NL = dot(N,L);
   float blinn = (NL <= 0) ? 0 : max(0, dot(N,H));
   
   return uSpecular * pow(blinn, uShininess);
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

float attenuation(float d) {
   return min(1.0, 1.0/(ATTENUATION_C1+d*ATTENUATION_C2+d*d*ATTENUATION_C3));
}

float isLitBySunPCF(vec3 N) {
   if (uDrawShadows == 0.0)
      return 1.0;
	  
   vec4 pLS = (vPosSunLS/vPosSunLS.w)*0.5+0.5;
   float storedDepth;
   float lit = 1.0;
   
   for(float x = 0.0; x < 5.0; x+=1.0) {
      for(float y = 0.0; y < 5.0; y+=1.0) {
         storedDepth = unpack(texture(uSunShadowmap, pLS.xy + vec2(-2.0+x,-2.0+y)/uSunShadowmapSize));
         if(storedDepth + BIAS_PCF_SUN < pLS.z )    
            lit  -= 1.0/25.0;
      }
   }
   
   return lit;
}

float isLitByLampPCF(int i, vec3 N) {
   if (uDrawShadows == 0.0)
   return 1.0;
	  
   vec4 pLS = (vPosLampLS[i]/vPosLampLS[i].w)*0.5+0.5;
   float storedDepth;
   float lit = 1.0;
   
   for(float x = 0.0; x < 5.0; x+=1.0) {
      for(float y = 0.0; y < 5.0; y+=1.0) {
         storedDepth = unpack(texture(uLampShadowmaps[i], pLS.xy + vec2(-2.0+x,-2.0+y)/uLampShadowmapSize));
         if(storedDepth + BIAS_PCF_LAMP < pLS.z )    
            lit  -= 1.0/25.0;
      }
   }
   
   return lit;
}

// use slope bias, as headlights are very close to the ground.
float isLitByHeadlightPCF(int i, vec3 N) {
   if (uDrawShadows == 0.0)
      return 1.0;
	  
   vec4 posLS = vPosHeadlightLS[i];
   vec4 pLS = (posLS/posLS.w)*0.5+0.5;
   float storedDepth;
   float lit = 1.0;
   
   float bias = clamp(BIAS_A*tanacos(dot(N,normalize(uHeadlightPos[i]))), BIAS_MIN_E, BIAS_MAX_E);
   for(float x = 0.0; x < 5.0; x+=1.0) {
      for(float y = 0.0; y < 5.0; y+=1.0) {
         storedDepth = unpack(texture(uHeadlightShadowmap[i], pLS.xy + vec2(-2.0+x,-2.0+y)/uHeadlightShadowmapSize));
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
   
   // textured flat shading
   if (uMode == 0) {
      surfaceNormal = normalize(cross(dFdx(vPosWS),dFdy(vPosWS)));
      sunIntensityDiff = diffuseIntensity(uSunDirection, surfaceNormal);
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
      sunIntensityDiff = diffuseIntensity(uSunDirection, surfaceNormal);
      sunIntensitySpec = specularIntensity(normalize(vSunVS-vPosVS), vNormalVS, normalize(-vPosVS));
      diffuseColor = texture2D(uColorImage,vTexCoord.xy);
   }
   // monochrome phong shading
   if (uMode == 3) {
      surfaceNormal = vNormalWS;
      sunIntensityDiff = diffuseIntensity(uSunDirection, surfaceNormal);
      sunIntensitySpec = specularIntensity(normalize(vSunVS-vPosVS), vNormalVS, normalize(-vPosVS));
      diffuseColor = vec4(uColor,1.0);
   }
   
   vec4 lampsContrib = vec4(0.0);
   vec4 headlightContrib = vec4(0.0);
   vec4 sunContrib = vec4(0.0);
   
   float sunint = sunlightIntensity();
   if (sunint > 0.0) {
      sunContrib = vec4(SUNLIGHT_COLOR,1.0) * sunint *
	               isLitBySunPCF(surfaceNormal) *
	               (sunIntensityDiff + sunIntensitySpec);
   }
   
   if(uLampState == 1.0) {
	  float lampsDiff = 0.0;
	  float lampsSpec = 0.0;
	  for (int i = 0; i < NUM_ACTIVE_LAMPS; ++i) {
		 // if the fragment is outside this lamp's light cone, skip all calculations
		 float spotint = spotlightIntensity(uLamps[i], vPosWS);
		 if (spotint == 0.0)
		    continue;

         lampsDiff += spotint * isLitByLampPCF(i, surfaceNormal);
		 lampsSpec += specularIntensity(normalize(vLampVS[i]-vPosVS), vNormalVS, normalize(-vPosVS));
      }
      lampsContrib = vec4(LAMPLIGHT_COLOR, 1.0) * (lampsDiff + lampsSpec);
   }

   if (uHeadlightState == 1.0) {
	  float headlightDiff = 0.0;
	  float headlightSpec = 0.0;
	  for (int i = 0; i < 2*NUM_CARS; ++i) {
		 // if the fragment is outside this headlight's light cone, skip all calculations
		 float headint = headlightIntensity(i);
		 if (headint == 0.0)
			continue;
		 headlightDiff += headint * attenuation(vPosHeadlightLS[i].w) *
							   isLitByHeadlightPCF(i, surfaceNormal);
		 headlightSpec += specularIntensity(normalize(vHeadlightVS[i]-vPosVS), vNormalVS, normalize(-vPosVS));
	   }
	   headlightContrib = vec4(HEADLIGHT_COLOR, 1.0) * (headlightDiff + headlightSpec);
   }

   color = diffuseColor * clamp(vec4(AMBIENT_LIGHT,1.0) + (lampsContrib + sunContrib + headlightContrib), 0.0, 1.0);
} 
