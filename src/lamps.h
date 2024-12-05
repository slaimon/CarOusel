#pragma once
#include "projector.h"

/*
   This class controls a group of lamps of which only a subset are active.
   Call the constructor, then toggle the lamps by calling the provided method.
   Turning lamps off is untested and pointless since limitations to GLSL mean
   you can't dinamically modify the length of a For loop anyway.

   After turning the lights on you can access their properties by using the
   provided methods. Make sure you use the correct index to access them:
   almost all methods use active-lamp indexing (e.g. index 0 corresponds to
   the first lamp you turned on), except toggle() which of course uses the
   actual index of the lamp.
*/

class LampGroup {
   protected:
      unsigned int size;
      unsigned int numActiveLamps;
      std::vector<SpotlightProjector> lampProjectors;
      std::vector<glm::vec3> lampPositions;
      std::vector<glm::mat4> lampMatrices;
      std::vector<int> lampTextureSlots;
      std::vector<bool> lampState;
      std::vector<unsigned int> activeLamps;

      bool sunlightSwitchState;
      bool userSwitchState;

      void updateActiveLampTable(unsigned int hasChanged) {
         // a lamp has turned on
         if (lampState[hasChanged]) {
            activeLamps[numActiveLamps] = hasChanged;
            ++numActiveLamps;
         }
         // a lamp has turned off
         else {
            bool found = false;
            unsigned int index;
            for (unsigned int i = 0; i < numActiveLamps; ++i)
               if (activeLamps[i] == hasChanged) {
                  found = true;
                  index = i;
                  break;
               }

            if (!found)
               return;

            activeLamps[index] = activeLamps[numActiveLamps - 1];
            --numActiveLamps;
         }
      }

   public:
      // initially all lamps are off
      LampGroup(std::vector<glm::vec3> positions, float angle_out, unsigned int shadowmap_size, int texture_slot_base) {
         size = positions.size();
         lampPositions = positions;

         lampProjectors.reserve(size);
         for (unsigned int i = 0; i < size; ++i)
            lampProjectors.emplace_back(shadowmap_size, positions[i], angle_out, glm::vec3(0.f, -1.f, 0.f));

         lampTextureSlots.resize(size);
         for (unsigned int i = 0; i < size; ++i) {
            int slot = texture_slot_base + i;
            lampTextureSlots[i] = slot;
            lampProjectors[i].bindTexture(slot);
         }

         lampMatrices.resize(size);
         for (unsigned int i = 0; i < size; ++i)
            lampMatrices[i] = lampProjectors[i].lightMatrix();

         lampState.resize(size);
         for (unsigned int i = 0; i < size; ++i)
            lampState[i] = false;

         activeLamps.resize(size);
         numActiveLamps = 0;
         sunlightSwitchState = false;
         userSwitchState = false;
      }

      // get total number of lamps
      unsigned int getSize() {
         return size;
      }

      // get the light matrix buffer, can be passed as uniform
      std::vector<glm::mat4> getLightMatrices() {
         std::vector<glm::mat4> result(size);
         for (unsigned int i = 0; i < numActiveLamps; ++i)
            result[i] = lampMatrices[activeLamps[i]];

         return result;
      }

      // get the positions buffer, can be passed as uniform
      std::vector<glm::vec3> getPositions() {
         std::vector<glm::vec3> result(size);
         for (unsigned int i = 0; i < numActiveLamps; ++i)
            result[i] = lampPositions[activeLamps[i]];

         return result;
      }

      // get the texture slot buffer, can be passed as uniform
      std::vector<int> getTextureSlots() {
         return lampTextureSlots;
      }

      // get the light matrix of the ith active lamp
      glm::mat4 getLightMatrix(unsigned int i) {
         return lampProjectors[getActiveLamp(i)].lightMatrix();
      }

      SpotlightProjector getProjector(unsigned int i) {
         return lampProjectors[getActiveLamp(i)];
      }

      // get the index of the ith active lamp
      unsigned int getActiveLamp(unsigned int i) {
         assert(i < numActiveLamps);
         return activeLamps[i];
      }

      // toggle ith lamp
      void toggle(unsigned int i) {
         assert(i < size);
         lampState[i] = !lampState[i];
         updateActiveLampTable(i);
      }

      // update the light matrix uniform of the ith active lamp
      void updateLightMatrixUniform(unsigned int i, shader s, const char* uniform_name) {
         lampProjectors[getActiveLamp(i)].updateLightMatrixUniform(s, uniform_name);
      }

      // bind the framebuffer of the ith active lamp
      void bindFramebuffer(unsigned int i) {
         lampProjectors[getActiveLamp(i)].bindFramebuffer();
      }

      // bind the texture of the ith active lamp
      void bindTexture(unsigned int i) {
         lampProjectors[getActiveLamp(i)].bindTexture(lampTextureSlots[i]);
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