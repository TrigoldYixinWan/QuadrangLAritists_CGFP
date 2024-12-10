#include "Camera.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

void Camera::updateViewMatrix() {
    // Compute the target point if not already computed
    target = position + look;

    // Compute view matrix manually
    glm::vec3 zAxis = glm::normalize(position - target); // Forward
    glm::vec3 xAxis = glm::normalize(glm::cross(glm::normalize(up), zAxis)); // Right
    glm::vec3 yAxis = glm::cross(zAxis, xAxis); // Up

    viewMatrix = glm::mat4(1.0f);

    viewMatrix[0][0] = xAxis.x;
    viewMatrix[1][0] = xAxis.y;
    viewMatrix[2][0] = xAxis.z;
    viewMatrix[0][1] = yAxis.x;
    viewMatrix[1][1] = yAxis.y;
    viewMatrix[2][1] = yAxis.z;
    viewMatrix[0][2] = zAxis.x;
    viewMatrix[1][2] = zAxis.y;
    viewMatrix[2][2] = zAxis.z;

    viewMatrix[3][0] = -glm::dot(xAxis, position);
    viewMatrix[3][1] = -glm::dot(yAxis, position);
    viewMatrix[3][2] = -glm::dot(zAxis, position);
}

void Camera::updateProjectionMatrix(int viewportWidth, int viewportHeight) {
    double aspectR = static_cast<double>(viewportWidth) / static_cast<double>(viewportHeight);
    double anglew = 2.0 * atan(tan(fovy / 2.0) * aspectR);

    double n = nearPlane;
    double f = farPlane;
    glm::mat4 M_scale = glm::mat4(
        1.0 / tan(anglew / 2.0), 0, 0, 0,
        0, 1.0 / tan(fovy / 2.0), 0, 0,
        0, 0, 1.0, 0,
        0, 0, 0, 1.0
        );

    glm::mat4 M_depth = glm::mat4(
        1.0, 0,    0,                            0,
        0,   1.0,  0,                            0,
        0,   0,   -(f + n) / (f - n),           -1.0,
        0,   0,   -(2.0 * f * n) / (f - n),      0
        );

    projectionMatrix = M_scale * M_depth;
}

