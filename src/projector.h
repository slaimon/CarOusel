#include <glm/glm.hpp>  
#include <glm/ext.hpp>

#include "transformations.h"
#include "../common/box3.h"
#include "../common/texture.h"
#include "../common/frame_buffer_object.h"

// Abstract Class; represents a light capable of casting shadows
class Projector {
   protected:
      glm::mat4 viewMatrix, projMatrix;
      box3 sceneBoundingBox;
      frame_buffer_object shadowmapFBO;
      unsigned int shadowmapSize;

      Projector(box3 box, unsigned int shadowmap_size) {
         viewMatrix = projMatrix = glm::mat4(1.0);
         sceneBoundingBox = box;
         shadowmapSize = shadowmap_size;
         shadowmapFBO.create(shadowmapSize, shadowmapSize);
      }

   public:
      glm::mat4 lightMatrix() {
         return projMatrix * viewMatrix;
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

      box3 getBoundingBox() {
         return sceneBoundingBox;
      }
};

// represents a directional light
class DirectionalProjector : public Projector {
   protected:
      glm::vec3 lightDirection;
      
      void update() {
         viewMatrix = lookAt(-lightDirection, glm::vec3(0.f), glm::vec3(0.,0.,1.f));
         
         // remove the translation component
         viewMatrix[3][0] = 0.f;
         viewMatrix[3][1] = 0.f;
         viewMatrix[3][2] = 0.f;

         // we transform the corners of the bounding box into the light's frame
         box3 aabb = transformBoundingBox(sceneBoundingBox, viewMatrix);
         
         // and that gives us the parameters of the light's view frustum
         float farPlane = -std::min(aabb.max.z, aabb.min.z);
         float nearPlane = -std::max(aabb.max.z, aabb.min.z);
         projMatrix =  glm::ortho(aabb.min.x, aabb.max.x, aabb.min.y, aabb.max.y, nearPlane, farPlane);
      }

   public:
      DirectionalProjector(box3 box, unsigned int shadowmap_size, glm::vec3 light_direction = glm::vec3(0.f,-1.f,0.f)) 
         : Projector(box, shadowmap_size) {
         setDirection(light_direction);
      }

      void setDirection(glm::vec3 light_direction) {
         lightDirection = glm::normalize(light_direction);
         update();
      }
};

// represents a positional light placed above the scene
class PositionalProjector : public Projector {
   protected:
      glm::vec3 lightPosition;

      void update() {
         viewMatrix = glm::lookAt(lightPosition, glm::vec3(lightPosition.x,0.0,lightPosition.z), glm::vec3(1.,0.,0.));
         box3 aabb = transformBoundingBox(sceneBoundingBox, viewMatrix);

         float nearPlane = 0.001;
         float farPlane = abs(aabb.min.z);
         projMatrix = glm::frustum(nearPlane * aabb.min.x / farPlane,
                                   nearPlane * aabb.max.x / farPlane,
                                   nearPlane * aabb.min.y / farPlane,
                                   nearPlane * aabb.max.y / farPlane,
                                   nearPlane, farPlane);
      }

   public:
      PositionalProjector(box3 box, unsigned int shadowmap_size, glm::vec3 light_position)
         : Projector(box, shadowmap_size) {
         setPosition(light_position);
      }

      void setPosition(glm::vec3 light_position) {
         lightPosition = light_position;
         update();
      }
};