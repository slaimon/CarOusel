#pragma once
#include <glm/glm.hpp>
#include <glm/ext.hpp>

typedef enum {
   CAMERA_AHEAD,
   CAMERA_BACK,
   CAMERA_LEFT,
   CAMERA_RIGHT,
   CAMERA_UP,
   CAMERA_DOWN
} cardinalDirection_t;

class CameraControls {
   private:
      glm::vec3 startPosition;
      glm::vec3 startTarget;

      float cameraSpeed;
      float mouseSensitivity;
      bool firstMouseLook;

      glm::vec3 cameraPosition;
      glm::vec3 cameraTarget;
      glm::vec3 cameraDirection; // these two should always be normalized
      glm::vec3 cameraRight;     //
      
      float cameraPitch;
      float cameraYaw;
      
      double lastX, lastY;
      
      inline glm::vec3 projectXZ(glm::vec3 v) {
         return glm::normalize(glm::vec3(v.x, 0.0f, v.z));
      }
   
   public:
      CameraControls(glm::vec3 pos, glm::vec3 target, float speed = 10.0, float sensitivity = 50.0) {
         startPosition = pos;
         startTarget = target;
         
         cameraSpeed = speed;
         mouseSensitivity = sensitivity;
         firstMouseLook = true;
         
         setView(pos, target);
      }
      
      void setView(glm::vec3 pos, glm::vec3 target) {
         cameraPosition = pos;
         cameraTarget = target;
         
         glm::vec3 diff = target-pos;
         cameraDirection = glm::normalize(-diff);
         cameraRight = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), cameraDirection));
         
         float distanceXZ = glm::length(glm::vec2(diff.x, diff.z));
         cameraPitch = glm::degrees(glm::atan(diff.y,distanceXZ));
         cameraYaw = glm::degrees(glm::atan(diff.z,diff.x)); // angle formed by the camera heading w.r.t. X+
         //std::cout<<"BEGIN: "<<cameraPitch<<"\t\t"<<cameraYaw<<std::endl;
      }
      
      void reset() {
         setView(startPosition, startTarget);
      }
      
      glm::mat4 matrix() {
         return glm::lookAt(cameraPosition, cameraPosition - cameraDirection, glm::vec3(0.0f, 1.0f, 0.0f));
      }
      
      glm::vec3 getPosition() {
         return cameraPosition;
      }
      
      void setSpeed(float newSpeed) {
         cameraSpeed = newSpeed;
      }
      
      void step(cardinalDirection_t direction, float delta) {
         float stepSize = cameraSpeed * delta;
         
         switch (direction) {
            case CAMERA_AHEAD:
               cameraPosition -= stepSize * projectXZ(cameraDirection);   // cameraDirection points to the BACK of the camera
               break;
            case CAMERA_BACK:
               cameraPosition += stepSize * projectXZ(cameraDirection);
               break;
            case CAMERA_LEFT:
               cameraPosition -= stepSize * cameraRight;
               break;
            case CAMERA_RIGHT:
               cameraPosition += stepSize * cameraRight;
               break;
            case CAMERA_UP:
               cameraPosition += stepSize * glm::vec3(0.0f, 1.0f, 0.0f);
               break;
            case CAMERA_DOWN:
               cameraPosition -= stepSize * glm::vec3(0.0f, 1.0f, 0.0f);
         }
      }
      
      void mouseLook(double xpos, double ypos) {
         if (firstMouseLook) {
            lastX = xpos;
            lastY = ypos;
            firstMouseLook = false;
            return;
         }
         float xoff = xpos - lastX;
         float yoff = lastY - ypos;
         
         lastX = xpos;
         lastY = ypos;
         
         cameraYaw += xoff * mouseSensitivity;
         cameraPitch += yoff * mouseSensitivity;
         
         if (cameraPitch > 89.0f)
            cameraPitch = 89.0f;
         if (cameraPitch < -89.0f)
            cameraPitch = -89.0f;
         
         glm::vec3 direction;
         direction.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
         direction.y = sin(glm::radians(cameraPitch));
         direction.z = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
         
         cameraDirection = -glm::normalize(direction);
         cameraRight = glm::normalize(glm::cross(direction, glm::vec3(0.0f, 1.0f, 0.0f)));
         //std::cout<<cameraPitch<<"\t\t"<<cameraYaw<<std::endl;
      }
};
