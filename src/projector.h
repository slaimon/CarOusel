#include <glm/glm.hpp>  
#include <glm/ext.hpp> 

#include "../common/box3.h"
#include "../common/texture.h"
#include "../common/frame_buffer_object.h"

// the Projector class represents a directional light
class Projector {
	   glm::mat4 viewMatrix, projMatrix;
	   box3 sceneBoundingBox;
	   glm::vec3 lightDirection;
	   unsigned int shadowmapSize;
	   frame_buffer_object shadowmapFBO;
	   
	   void update() {
	      viewMatrix = lookAt(-lightDirection, glm::vec3(0.f), glm::vec3(0.,0.,1.f));
		   box3 box = sceneBoundingBox;
		   box3 aabb(glm::vec3(0.f),glm::vec3(0.f));
		   
		   // we transform the corners of the bounding box into the light's frame
		   glm::vec4 corners[8];
		   corners[0] = glm::vec4(box.min, 1.0);
		   corners[1] = glm::vec4(box.min.x, box.min.y, box.max.z, 1.0);
		   corners[2] = glm::vec4(box.min.x, box.max.y, box.max.z, 1.0);
		   corners[3] = glm::vec4(box.min.x, box.max.y, box.min.z, 1.0);
		   corners[4] = glm::vec4(box.max.x, box.max.y, box.min.z, 1.0);
		   corners[5] = glm::vec4(box.max.x, box.min.y, box.min.z, 1.0);
		   corners[6] = glm::vec4(box.max.x, box.min.y, box.max.z, 1.0);
		   corners[7] = glm::vec4(box.max, 1.0);
		   
		   // minus the translation
		   viewMatrix[3][0] = 0.f;
		   viewMatrix[3][1] = 0.f;
		   viewMatrix[3][2] = 0.f;
		   
		   for (int i=0; i<8; i++)
		      aabb.add(glm::vec3(viewMatrix*corners[i]));
		   
		   // and that gives us the parameters of the light's view frustum
		   float farPlane = std::max(-aabb.max.z, -aabb.min.z);
		   float nearPlane = std::min(-aabb.max.z, -aabb.min.z);
		   projMatrix =  glm::ortho(aabb.min.x, aabb.max.x, aabb.min.y, aabb.max.y, nearPlane, farPlane);
	   }

   public:
	   Projector(box3 box, unsigned int shadowmap_size, glm::vec3 light_direction = glm::vec3(0.f,-1.f,0.f)) {
	      sceneBoundingBox = box;
	      lightDirection = glm::normalize(light_direction);
		  shadowmapSize = shadowmap_size;
		  shadowmapFBO.create(shadowmapSize, shadowmapSize);
	   }

	   void setDirection(glm::vec3 light_direction) {
	      lightDirection = glm::normalize(light_direction);
	      update();
	   }
	   
	   glm::mat4 lightMatrix() {
		   return projMatrix*viewMatrix;
	   }

	   unsigned int getShadowmapSize() {
		   return shadowmapSize;
	   }

	   unsigned int getTextureID() {
		   return shadowmapFBO.id_tex;
	   }

	   unsigned int getFrameBufferID() {
		   return shadowmapFBO.id_fbo;
	   }
};
