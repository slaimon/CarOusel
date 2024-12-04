#pragma once
#include <glm/glm.hpp>
#include <glm/ext.hpp>

// implements the headlights for one car
class Headlights {
   protected:
      glm::mat4 lightMatrix[2];
      glm::mat4 projMatrix;
      glm::mat4 carToWorld;
      glm::mat4 headlightTransform[2];

   public:
      Headlights(float opening_angle, glm::vec3 car_frame_origin, float car_frame_scale) {
         lightMatrix[0] = glm::mat4(1.f);
         lightMatrix[1] = glm::mat4(1.f);
         projMatrix = glm::perspective(opening_angle, 1.f, 0.001f / car_frame_scale, 1.f / car_frame_scale);

         carToWorld = glm::translate(glm::scale(glm::mat4(1.f), glm::vec3(car_frame_scale)), -car_frame_origin);
         glm::mat4 R = glm::rotate(glm::radians(-5.f), glm::vec3(1.f, 0.f, 0.f));
         headlightTransform[0] = glm::translate(R, glm::vec3( 0.45f, 0.5f, -1.25f));
         headlightTransform[1] = glm::translate(R, glm::vec3(-0.45f, 0.5f, -1.25f));
      }

      void setCarFrame(glm::mat4 F) {
         glm::mat4 M = carToWorld * F;
         lightMatrix[0] = projMatrix * glm::inverse(M * headlightTransform[0]);
         lightMatrix[1] = projMatrix * glm::inverse(M * headlightTransform[1]);
      }

      glm::mat4 getMatrix(int i) {
         assert(i == 0 || i == 1);
         return lightMatrix[i];
      }

      // s.program must be in use
      void updateLightMatrixUniform(shader s, const char* uniform_name) {
         glUniformMatrix4fv(s[uniform_name], 2, GL_FALSE, &lightMatrix[0][0][0]);
      }
};