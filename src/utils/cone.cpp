#include "Cone.h"
#include "glm/ext/scalar_constants.hpp"

void Cone::updateParams(int param1, int param2) {
    m_param1 = param1; // Should control height subdivisions
    m_param2 = param2; // Should control radial subdivisions
    setVertexData();
}

void Cone::setVertexData() {
    m_vertexData.clear();

    const float PI = glm::pi<float>();
    float r = 0.5f;   // Base radius
    float h = 1.0f;   // Height of the cone

    int numSlices = std::max(3, m_param2);  // Minimum of 3 slices to form a cone
    int numStacks = std::max(1, m_param1);  // At least one stack

    // **Generate the Base**

    // Center of the base
    glm::vec3 baseCenter(0.0f, -0.5f * h, 0.0f);
    glm::vec3 baseNormal(0.0f, -1.0f, 0.0f);

    for (int i = 0; i < numSlices; ++i) {
        float theta0 = (2.0f * PI * i) / numSlices;
        float theta1 = (2.0f * PI * (i + 1)) / numSlices;

        // Positions of the outer edge vertices
        glm::vec3 p0(r * cos(theta0), -0.5f * h, r * sin(theta0));
        glm::vec3 p1(r * cos(theta1), -0.5f * h, r * sin(theta1));

        // Triangle: baseCenter, p0, p1
        insertVec3(m_vertexData, baseCenter);
        insertVec3(m_vertexData, baseNormal);

        insertVec3(m_vertexData, p0);
        insertVec3(m_vertexData, baseNormal);

        insertVec3(m_vertexData, p1);
        insertVec3(m_vertexData, baseNormal);
    }

    // **Generate the Side Surface**

    // Precompute the slant height and side normals
    float slantHeight = sqrt(h * h + r * r);
    float ny = r / slantHeight;
    float nxz = h / slantHeight;  // Since nx and nz are proportional to x and z positions

    // For each stack (vertical subdivision)
    for (int j = 0; j < numStacks; ++j) {
        float y0 = -0.5f * h + (h * j) / numStacks;
        float y1 = -0.5f * h + (h * (j + 1)) / numStacks;

        float r0 = r * (1.0f - static_cast<float>(j) / numStacks);
        float r1 = r * (1.0f - static_cast<float>(j + 1) / numStacks);

        // For each slice around the cone
        for (int i = 0; i < numSlices; ++i) {
            float theta0 = (2.0f * PI * i) / numSlices;
            float theta1 = (2.0f * PI * (i + 1)) / numSlices;

            // Positions of the vertices
            glm::vec3 p0(r0 * cos(theta0), y0, r0 * sin(theta0));
            glm::vec3 p1(r0 * cos(theta1), y0, r0 * sin(theta1));
            glm::vec3 p2(r1 * cos(theta0), y1, r1 * sin(theta0));
            glm::vec3 p3(r1 * cos(theta1), y1, r1 * sin(theta1));

            // Normals at the vertices
            glm::vec3 n0 = glm::normalize(glm::vec3(cos(theta0) * nxz, ny, sin(theta0) * nxz));
            glm::vec3 n1 = glm::normalize(glm::vec3(cos(theta1) * nxz, ny, sin(theta1) * nxz));
            glm::vec3 n2 = n0;
            glm::vec3 n3 = n1;

            // Triangle 1: p0, p2, p1
            insertVec3(m_vertexData, p0);
            insertVec3(m_vertexData, n0);

            insertVec3(m_vertexData, p2);
            insertVec3(m_vertexData, n2);

            insertVec3(m_vertexData, p1);
            insertVec3(m_vertexData, n1);

            // Triangle 2: p1, p2, p3
            insertVec3(m_vertexData, p1);
            insertVec3(m_vertexData, n1);

            insertVec3(m_vertexData, p2);
            insertVec3(m_vertexData, n2);

            insertVec3(m_vertexData, p3);
            insertVec3(m_vertexData, n3);
        }
    }
}


// Inserts a glm::vec3 into a vector of floats.
// This will come in handy if you want to take advantage of vectors to build your shape!
void Cone::insertVec3(std::vector<float> &data, glm::vec3 v) {
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
}
