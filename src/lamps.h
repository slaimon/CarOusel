#pragma once
#include "projector.h"

class LampGroup {
   public:
      unsigned int size;
      unsigned int numActiveLamps;
      std::vector<SpotlightProjector> lampProjectors;
      std::vector<glm::vec3> lampPositions;
      std::vector<glm::mat4> lampMatrices;
      std::vector<int> lampTextureSlots;
      std::vector<bool> lampState;
      std::vector<unsigned int> activeLamps;

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
      }

      glm::mat4 lightMatrix(unsigned int i) {
         return lampProjectors[getActiveLamp(i)].lightMatrix();
      }

      unsigned int getSize() {
         return size;
      }

      unsigned int getActiveLamp(unsigned int i) {
         assert(i < numActiveLamps);
         return activeLamps[i];
      }

      std::vector<glm::mat4> getLightMatrices() {
         std::vector<glm::mat4> result(size);
         for (unsigned int i = 0; i < numActiveLamps; ++i)
            result[i] = lampMatrices[activeLamps[i]];

         return result;
      }

      std::vector<glm::vec3> getPositions() {
         std::vector<glm::vec3> result(size);
         for (unsigned int i = 0; i < numActiveLamps; ++i)
            result[i] = lampPositions[activeLamps[i]];

         return result;
      }

      std::vector<int> getTextureSlots() {
         return lampTextureSlots;
      }

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

      void toggle(unsigned int i) {
         assert(i < size);
         lampState[i] = !lampState[i];
         updateActiveLampTable(i);
      }

      void updateLightMatrixUniform(unsigned int i, shader s, const char* uniform_name) {
         lampProjectors[getActiveLamp(i)].updateLightMatrixUniform(s, uniform_name);
      }

      void bindFramebuffer(unsigned int i) {
         lampProjectors[getActiveLamp(i)].bindFramebuffer();
      }

      void bindTexture(unsigned int i) {
         lampProjectors[getActiveLamp(i)].bindTexture(lampTextureSlots[i]);
      }
};