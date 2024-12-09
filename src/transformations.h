#pragma once
#include <iostream>
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>

#include "common/box3.h"
#include "common/carousel/carousel.h"

// returns a vector containing the 8 corners of a box3 object
inline std::vector<glm::vec4> getBoxCorners(box3 box) {
   std::vector<glm::vec4> corners(8);

   corners[0] = glm::vec4(box.min, 1.0);
   corners[1] = glm::vec4(box.min.x, box.min.y, box.max.z, 1.0);
   corners[2] = glm::vec4(box.min.x, box.max.y, box.max.z, 1.0);
   corners[3] = glm::vec4(box.min.x, box.max.y, box.min.z, 1.0);
   corners[4] = glm::vec4(box.max.x, box.max.y, box.min.z, 1.0);
   corners[5] = glm::vec4(box.max.x, box.min.y, box.min.z, 1.0);
   corners[6] = glm::vec4(box.max.x, box.min.y, box.max.z, 1.0);
   corners[7] = glm::vec4(box.max, 1.0);

   return corners;
}

// returns an approximation for the bounding box under the transformation T
inline box3 transformBoundingBox(box3 box, glm::mat4 T) {
   box3 aabb(0.0);

   std::vector<glm::vec4> corners = getBoxCorners(box);
   for (int i = 0; i < 8; i++)
      aabb.add(glm::vec3(T * corners[i]));

   return aabb;
}

// returns the vector pointing from the given point to the curb vertex closest to it
inline glm::vec3 findClosestCurbVertex(track t, glm::vec3 lamp_position) {
    std::vector<glm::vec3> curbs = t.curbs[0];
    float min_dist = 9e99;
    glm::vec3 closest = glm::vec3(0.f);
    for (unsigned int i = 0; i < curbs.size(); i++) {
        float dist = glm::length(lamp_position - curbs[i]);
        if (dist < min_dist) {
            min_dist = dist;
            closest = curbs[i];
        }
    }

    glm::vec3 diff = lamp_position - closest;
    return glm::normalize(glm::vec3(diff.x, 0.f, diff.z));
}

// returns a rotation matrix that maps vector a to vector b
inline glm::mat4 alignVector(glm::vec3 a, glm::vec3 b) {
    float angle = glm::acos(glm::dot(a, b));
    glm::vec3 axis = glm::cross(a, b);
    // rotate around the axis cross(x,c) by an angle acos(dot(x,c))
    return  glm::rotate(glm::mat4(1.f), angle, axis);
}

// returns a vector containing the direction each lamp should face as a rotation matrix
inline std::vector<glm::mat4> computeLampOrientation(track t, std::vector<stick_object> lamps) {
    std::vector<glm::mat4> result(lamps.size());
    for (unsigned int i = 0; i < result.size(); i++) {
        glm::vec3 closest = findClosestCurbVertex(t, lamps[i].pos);
        result[i] = alignVector(glm::vec3(1., 0., 0.f), closest);
    }

    return result;
}

// returns a vector containing the transformation to be applied to each lamp
inline std::vector<glm::mat4> lampTransform(track t, std::vector<stick_object> lamps, float scale, glm::vec3 center) {
    std::vector<glm::mat4> result(lamps.size());
    std::vector<glm::mat4> rotations = computeLampOrientation(t, lamps);

    glm::mat4 S = glm::scale(glm::mat4(1.f), glm::vec3(1. / 20.f));
    glm::mat4 T = glm::translate(glm::mat4(1.f), glm::vec3(-0.1, 0.48f, 0.));
    for (unsigned int i = 0; i < result.size(); i++) {
        glm::mat4 T2 = glm::translate(glm::mat4(1.f), scale * (lamps[i].pos - center));
        result[i] = T2 * rotations[i] * S * T;
    }

    return result;
}

// given the vector returned by lampTransform, returns the position of each lamp's lightbulb
inline std::vector<glm::vec3> lampLightPositions(std::vector<glm::mat4> lampTransforms) {
    std::vector<glm::vec3> result(lampTransforms.size());
    glm::mat4 T = glm::translate(glm::mat4(1.f), glm::vec3(-0.1, 0.45, 0.f));
    for (unsigned int i = 0; i < result.size(); i++)
        result[i] = lampTransforms[i] * T * glm::vec4(0.f, 0.f, 0.f, 1.f);

    return result;
}

inline float random(float low, float high) {
    return low + (high - low) * (std::rand() / (float)RAND_MAX);
}

inline glm::mat4 randomRotation() {
    float angle = random(0.f, 6.28f);
    return glm::rotate(glm::mat4(1.f), angle, glm::vec3(0.f, 1.f, 0.f));
}

inline std::vector<glm::mat4> treeTransform(std::vector<stick_object> trees, float scale, glm::vec3 center) {
    std::vector<glm::mat4> result(trees.size());

    glm::mat4 T = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.43f, 0.f));
    for (unsigned int i = 0; i < result.size(); i++) {
        glm::mat4 T2 = glm::translate(glm::mat4(1.f), scale * (trees[i].pos - center));
        glm::mat4 R = randomRotation();
        glm::mat4 S = glm::scale(glm::mat4(1.f), glm::vec3(1. / 12.f) * random(0.8f, 1.2f));
        result[i] = T2 * R * S * T;
    }

    return result;
}