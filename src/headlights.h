#pragma once
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "projector.h"

// implements the headlights for one car
class Headlights {
   protected:
      glm::mat4 lightMatrix[2];
      glm::vec3 position[2];
      glm::mat4 projMatrix;
      glm::mat4 carToWorld;
      std::vector<HeadlightProjector> projector;

      bool sunlightSwitchState;
      bool userSwitchState;

      void updateLightMatrix() {
         lightMatrix[0] = projector[0].lightMatrix();
         lightMatrix[1] = projector[1].lightMatrix();
      }

      void updatePosition() {
         position[0] = projector[0].getPosition();
         position[1] = projector[1].getPosition();
      }

   public:
      Headlights(float opening_angle, glm::vec3 car_frame_origin, float car_frame_scale, unsigned int shadowmap_size) {
         lightMatrix[0] = glm::mat4(1.f);
         lightMatrix[1] = glm::mat4(1.f);
         projMatrix = glm::perspective(opening_angle/3.f, 3.f, 0.005f / car_frame_scale, 0.5f / car_frame_scale);

         carToWorld = glm::translate(glm::scale(glm::mat4(1.f), glm::vec3(car_frame_scale)), -car_frame_origin);
         glm::mat4 R = glm::rotate(glm::radians(-5.f), glm::vec3(1.f, 0.f, 0.f));

         projector.reserve(2);
         projector.emplace_back(shadowmap_size, glm::translate(glm::vec3(0.45f, 0.5f, -1.25f))*R, projMatrix);
         projector.emplace_back(shadowmap_size, glm::translate(glm::vec3(-0.45f, 0.5f, -1.25f))*R, projMatrix);

         sunlightSwitchState = false;
         userSwitchState = false;
         updatePosition();
      }

      void setCarFrame(glm::mat4 F) {
         glm::mat4 M = carToWorld * F;
         projector[0].setCarTransform(M);
         projector[1].setCarTransform(M);
      }

      glm::mat4 getMatrix(int i) {
         assert(i == 0 || i == 1);
         return projector[i].lightMatrix();
      }

      void bindFramebuffer(int i) {
         assert(i == 0 || i == 1);
         projector[i].bindFramebuffer();
      }

      void updateLightMatrixUniform(int i, shader s, const char* uniform_name) {
         assert(i == 0 || i == 1);
         projector[i].updateLightMatrixUniform(s, uniform_name);
      }

      void bindTexture(int i, int texture_slot) {
         assert(i == 0 || i == 1);
         projector[i].bindTexture(texture_slot);
      }

      // s.program must be in use
      void updateLightMatrixUniformArray(shader s, const char* uniform_name) {
         updateLightMatrix();
         glUniformMatrix4fv(s[uniform_name], 2, GL_FALSE, &lightMatrix[0][0][0]);
      }

      // s.program must be in use
      void updatePositionUniformArray(shader s, const char* uniform_name) {
         updatePosition();
         glUniform3fv(s[uniform_name], 2, &position[0][0]);
      }

      int getTextureID(int i) {
         assert(i == 0 || i == 1);
         return projector[i].getTextureID();
      }

      // set the lamp status according to the current sun position
      void setSunlightSwitch(glm::vec3 sunlight_direction, float nighttime_threshold = 0.15f) {
         if (dot(normalize(sunlight_direction), glm::vec3(0.f, 1.f, 0.f)) <= nighttime_threshold)
            sunlightSwitchState = true;
         else
            sunlightSwitchState = false;
      }

      // set the user-defined lamp status
      void setUserSwitch(bool state) {
         userSwitchState = state;
      }

      bool isOn() {
         return userSwitchState || sunlightSwitchState;
      }
};