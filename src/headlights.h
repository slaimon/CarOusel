#pragma once
#include <glm/glm.hpp>
#include <glm/ext.hpp>

// implements the headlights for one car
class Headlights {
   protected:
      glm::mat4 viewMatrix[2];
      glm::mat4 projMatrix;
      glm::vec3 carFrameOrigin;
      float carFrameScale;
      glm::mat4 headlightTransform[2];

   public:
      Headlights(float opening_angle, glm::vec3 car_frame_origin, float car_frame_scale) {
         viewMatrix[0] = glm::lookAt(glm::vec3(0.f), glm::vec3(0.f, -1.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
         viewMatrix[1] = glm::lookAt(glm::vec3(0.f), glm::vec3(0.f, -1.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
         projMatrix = glm::perspective(opening_angle, 1.f, 0.001f, 1.f);

         carFrameOrigin = car_frame_origin;
         carFrameScale = car_frame_scale;
         headlightTransform[0] = glm::translate(glm::mat4(1.f), glm::vec3( 0.45f, 0.5f, -1.25f));
         headlightTransform[1] = glm::translate(glm::mat4(1.f), glm::vec3(-0.45f, 0.5f, -1.25f));
      }

      void setCarFrame(glm::mat4 F) {
         glm::mat4 F0 = F * headlightTransform[0];
         glm::mat4 F1 = F * headlightTransform[1];
         glm::mat4 T = glm::translate(glm::mat4(1.f), -carFrameOrigin * carFrameScale);
         glm::mat4 SM = glm::translate(glm::scale(glm::mat4(1.f), glm::vec3(carFrameScale)), -carFrameOrigin);
         viewMatrix[0] = glm::inverse(glm::scale(SM * F0, glm::vec3(1.f / carFrameScale)));
         viewMatrix[1] = glm::inverse(glm::scale(SM * F1, glm::vec3(1.f / carFrameScale)));
      }

      glm::mat4 getMatrix(int i) {
         assert(i == 0 || i == 1);
         return projMatrix * viewMatrix[i];
      }
};