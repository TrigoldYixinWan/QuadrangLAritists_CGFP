#pragma once

#include <glm/glm.hpp>

class Camera {
public:
    glm::vec3 position;
    glm::vec3 target;   // Computed as position + look
    glm::vec3 up;

    glm::vec3 look;     // Direction the camera is looking at

    float fovy;
    float aspect;
    float nearPlane;
    float farPlane;

    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;

    void updateViewMatrix();
    void updateProjectionMatrix(int viewportWidth, int viewportHeight);
};
