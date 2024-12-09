#pragma once
#include <glm/glm.hpp>  
#include <glm/ext.hpp>

#include "transformations.h"
#include "common/box3.h"
#include "common/texture.h"
#include "common/frame_buffer_object.h"

// Abstract Class; represents a light capable of casting shadows
class Projector {
   protected:
      glm::mat4 viewMatrix, projMatrix;
      frame_buffer_object shadowmapFBO;
      unsigned int shadowmapSize;

      Projector(unsigned int shadowmap_size) {
         viewMatrix = projMatrix = glm::mat4(1.0);
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

      void bindFramebuffer() {
         glBindFramebuffer(GL_FRAMEBUFFER, shadowmapFBO.id_fbo);
         glViewport(0, 0, shadowmapSize, shadowmapSize);
         glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
      }

      void bindTexture(int texture_slot) {
         glActiveTexture(GL_TEXTURE0 + texture_slot);
         glBindTexture(GL_TEXTURE_2D, shadowmapFBO.id_tex);
      }

      // s.program must be in use
      inline void updateLightMatrixUniform(shader s, const char* uniform_name) {
         glUniformMatrix4fv(s[uniform_name], 1, GL_FALSE, &(projMatrix * viewMatrix)[0][0]);
      }
};

// represents a car's headlight
class HeadlightProjector : public Projector {
   protected:
      glm::mat4 headlightTransform;
      glm::vec3 headlightPosition;

   public:
      HeadlightProjector(unsigned int shadowmap_size, glm::mat4 headlight_transform, glm::mat4 proj_matrix)
         : Projector(shadowmap_size) {
         headlightTransform = headlight_transform;
         projMatrix = proj_matrix;
         headlightPosition = glm::vec3(0.f);
      }

      void setCarTransform(glm::mat4 car_transform) {
         glm::mat4 M = car_transform * headlightTransform;
         headlightPosition = glm::vec3(M[3]);

         viewMatrix = glm::inverse(car_transform * headlightTransform);
      }

      glm::vec3 getPosition() {
         return headlightPosition;
      }
};

// represents a directional light
class DirectionalProjector : public Projector {
   protected:
      glm::vec3 lightDirection;
      box3 sceneBoundingBox;
      
      void update() {
         viewMatrix = lookAt(lightDirection, glm::vec3(0.f), glm::vec3(0.,0.,1.f));
         
         // remove the translation component
         viewMatrix[3][0] = 0.f;
         viewMatrix[3][1] = 0.f;
         viewMatrix[3][2] = 0.f;

         // we transform the corners of the bounding box into the light's frame
         box3 aabb = transformBoundingBox(sceneBoundingBox, viewMatrix);
         
         // and that gives us the parameters of the light's view frustum
         projMatrix =  glm::ortho(aabb.min.x, aabb.max.x, aabb.min.y, aabb.max.y, -aabb.max.z, -aabb.min.z);
      }

   public:
      DirectionalProjector(box3 box, unsigned int shadowmap_size, glm::vec3 light_direction = glm::vec3(0.f,-1.f,0.f)) 
         : Projector(shadowmap_size) {
         sceneBoundingBox = box;
         setDirection(light_direction);
      }

      void setDirection(glm::vec3 light_direction) {
         lightDirection = glm::normalize(light_direction);
         update();
      }

      glm::vec3 getDirection() {
         return lightDirection;
      }

      box3 getBoundingBox() {
         return sceneBoundingBox;
      }

      inline void updateLightDirectionUniform(shader s, const char* uniform_name) {
         glUniform3f(s[uniform_name], lightDirection.x, lightDirection.y, lightDirection.z);
      }
};

// represents a spotlight capable of casting shadows
class SpotlightProjector : public Projector {
   protected:
      glm::vec3 lightPosition;
      glm::vec3 lightDirection;
      float lightAngle_out;

      void update() {
         viewMatrix[0] = glm::vec4(1.f, 0.f, 0.f, 0.f);
         viewMatrix[1] = glm::vec4(glm::cross(-lightDirection, glm::vec3(1.f, 0.f, 0.f)), 0.f);
         viewMatrix[2] = glm::vec4(-lightDirection, 0.f);
         viewMatrix[3] = glm::vec4(lightPosition, 1.f);
         viewMatrix = glm::inverse(viewMatrix);

         float nearPlane = 0.001;
         float farPlane = 0.07;
         projMatrix = glm::perspective(2.0f*lightAngle_out, 1.0f, nearPlane, farPlane);
      }

   public:
      SpotlightProjector(unsigned int shadowmap_size, glm::vec3 light_position, float angle_out, glm::vec3 direction)
         : Projector(shadowmap_size) {
         lightPosition = light_position;
         lightDirection = direction;
         lightAngle_out = angle_out;
         update();
      }
};