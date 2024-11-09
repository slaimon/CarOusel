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
    glm::vec3 upVector;
    float cameraSpeed;
    float mouseSensitivity;
    bool firstMouseLook;

    glm::vec3 cameraPosition;
    glm::vec3 cameraRight;     // these two should always be normalized:
    glm::vec3 cameraHeading;   //

    float cameraPitch;
    float cameraYaw;

    double lastX, lastY;

    inline glm::vec3 projectXZ(glm::vec3 v) {
        return glm::normalize(glm::vec3(v.x, 0.0f, v.z));
    }


public:

    CameraControls(glm::vec3 pos, glm::vec3 target, float speed = 10.0, float sensitivity = 50.0) {
        upVector = glm::vec3(0.0f, 1.0f, 0.0f);
        cameraSpeed = speed;
        mouseSensitivity = sensitivity;
        firstMouseLook = true;

        setView(pos, target);
    }

    void setView(glm::vec3 pos, glm::vec3 target) {
        cameraPosition = pos;

        glm::vec3 diff = pos - target;
        if (glm::length(diff) == 0.f)
            diff = glm::vec3(0.f, 0.f, -1.f);

        if (abs(glm::dot(upVector, glm::normalize(diff))) == 1.f) {  // if the camera is looking straight up or down, set Z- as the heading
            cameraHeading = glm::vec3(0.f, 0.f, -1.f);
            cameraRight = glm::vec3(1.f, 0.f, 0.f);
        }
        else {
            cameraHeading = -glm::normalize(glm::vec3(diff.x, 0.f, diff.z));
            cameraRight = glm::normalize(glm::cross(cameraHeading, upVector));
        }

        // calculate pitch and yaw angles
        cameraPitch = glm::degrees(-glm::asin(diff.y / glm::length(diff)));
        if (diff.x == 0.f && diff.z == 0)
            cameraYaw = 0.f;
        else
            cameraYaw = glm::degrees(glm::atan(diff.x,diff.z));
    }

    glm::mat4 matrix() {
        glm::mat4 pitchRotation = glm::rotate(glm::mat4(1.f), -glm::radians(cameraPitch), glm::vec3(1.f, 0.f, 0.f));
        glm::mat4 yawRotation = glm::rotate(pitchRotation, -glm::radians(cameraYaw), upVector);
        return glm::translate(yawRotation, -cameraPosition);
    }

    void setSpeed(float newSpeed) {
        cameraSpeed = newSpeed;
    }

    glm::vec3 getPosition() {
        return cameraPosition;
    }

    void step(cardinalDirection_t direction, float delta) {
        float stepSize = cameraSpeed * delta;

        switch (direction) {
        case CAMERA_AHEAD:
            cameraPosition += stepSize * cameraHeading;
            break;
        case CAMERA_BACK:
            cameraPosition -= stepSize * cameraHeading;
            break;
        case CAMERA_LEFT:
            cameraPosition -= stepSize * cameraRight;
            break;
        case CAMERA_RIGHT:
            cameraPosition += stepSize * cameraRight;
            break;
        case CAMERA_UP:
            cameraPosition += stepSize * upVector;
            break;
        case CAMERA_DOWN:
            cameraPosition -= stepSize * upVector;
        }
    }

    void mouseLook(double xpos, double ypos) {
        if (firstMouseLook) {
            lastX = xpos;
            lastY = ypos;
            firstMouseLook = false;
            return;
        }
        float xoff = lastX - xpos;
        float yoff = lastY - ypos;

        lastX = xpos;
        lastY = ypos;

        cameraYaw += xoff * mouseSensitivity;
        cameraPitch += yoff * mouseSensitivity;

        if (cameraPitch > 89.0f)
            cameraPitch = 89.0f;
        if (cameraPitch < -89.0f)
            cameraPitch = -89.0f;

        cameraHeading = -glm::vec3(glm::sin(glm::radians(cameraYaw)), 0.f, glm::cos(glm::radians(cameraYaw)));
        cameraRight = glm::normalize(glm::cross(cameraHeading, upVector));
    }
};
