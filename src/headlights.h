#pragma once
#include <glm/glm.hpp>
#include <glm/ext.hpp>

// implements the headlights for one car
class Headlights {
   protected:
      glm::mat4 hlViewMatrix[2];
      glm::mat4 hlProjMatrix;
      glm::vec3 hlCarFrameOrigin;
      float hlCarFrameScale;
      glm::mat4 hlHeadlightTransform[2];

   public:
      Headlights(float opening_angle, glm::vec3 car_frame_origin, float car_frame_scale) {
         hlViewMatrix[0] = glm::lookAt(glm::vec3(0.f), glm::vec3(1.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
         hlViewMatrix[1] = glm::lookAt(glm::vec3(0.f), glm::vec3(1.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
         hlProjMatrix = glm::perspective(opening_angle, 1.f, 0.001f, 1.f);

         hlCarFrameOrigin = car_frame_origin;
         hlCarFrameScale = car_frame_scale;
         hlHeadlightTransform[0] = glm::translate(glm::mat4(1.f), glm::vec3( 0.45f, 0.5f, -1.25f));
         hlHeadlightTransform[1] = glm::translate(glm::mat4(1.f), glm::vec3(-0.45f, 0.5f, -1.25f));
      }

      void setCarFrame(glm::mat4 F) {
         glm::mat4 F1 = glm::translate(F, glm::vec3(0.45f, 0.5f, -1.25f));
         glm::mat4 F2 = glm::translate(F, glm::vec3(-0.45f, 0.5f, -1.25f));
         glm::mat4 SM = glm::translate(glm::scale(glm::mat4(1.f), glm::vec3(hlCarFrameScale)), -hlCarFrameOrigin);
         hlViewMatrix[0] = glm::inverse(glm::scale(SM * F1, glm::vec3(1.f / hlCarFrameScale)));
         hlViewMatrix[1] = glm::inverse(glm::scale(SM * F2, glm::vec3(1.f / hlCarFrameScale)));
      }

      glm::mat4 getMatrix(int i) {
         assert(i == 0 || i == 1);
         return hlProjMatrix * hlViewMatrix[i];
      }
};