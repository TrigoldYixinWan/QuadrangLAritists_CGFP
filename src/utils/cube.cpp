#include "Cube.h"

void Cube::updateParams(int param1) {
    m_vertexData = std::vector<float>();
    m_param1 = param1;
    setVertexData();
}

void Cube::makeTile(glm::vec3 topLeft,
                    glm::vec3 topRight,
                    glm::vec3 bottomLeft,
                    glm::vec3 bottomRight) {
    // Task 2: create a tile (i.e. 2 triangles) based on 4 given points.
    glm::vec3 edge1 = bottomLeft - topLeft;
    glm::vec3 edge2 = topRight - topLeft;
    glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

    // First Triangle (topLeft, bottomLeft, topRight)
    insertVec3(m_vertexData, topLeft);
    insertVec3(m_vertexData, normal);

    insertVec3(m_vertexData, bottomLeft);
    insertVec3(m_vertexData, normal);

    insertVec3(m_vertexData, topRight);
    insertVec3(m_vertexData, normal);

    // Second Triangle (bottomLeft, bottomRight, topRight)
    insertVec3(m_vertexData, bottomLeft);
    insertVec3(m_vertexData, normal);

    insertVec3(m_vertexData, bottomRight);
    insertVec3(m_vertexData, normal);

    insertVec3(m_vertexData, topRight);
    insertVec3(m_vertexData, normal);
}

void Cube::makeFace(glm::vec3 topLeft,
                    glm::vec3 topRight,
                    glm::vec3 bottomLeft,
                    glm::vec3 bottomRight) {
    // Task 3: create a single side of the cube out of the 4
    //         given points and makeTile()
    // Note: think about how param 1 affects the number of triangles on
    //       the face of the cube
    int subdivisions = std::max(1, m_param1);

    for (int i = 0; i < subdivisions; ++i) {
        float v0 = static_cast<float>(i) / subdivisions;
        float v1 = static_cast<float>(i + 1) / subdivisions;

        // Interpolate along the left and right edges
        glm::vec3 leftEdgeTop = glm::mix(topLeft, bottomLeft, v0);
        glm::vec3 leftEdgeBottom = glm::mix(topLeft, bottomLeft, v1);
        glm::vec3 rightEdgeTop = glm::mix(topRight, bottomRight, v0);
        glm::vec3 rightEdgeBottom = glm::mix(topRight, bottomRight, v1);

        for (int j = 0; j < subdivisions; ++j) {
            float u0 = static_cast<float>(j) / subdivisions;
            float u1 = static_cast<float>(j + 1) / subdivisions;

            // Calculate the four corners of the tile
            glm::vec3 topLeftTile = glm::mix(leftEdgeTop, rightEdgeTop, u0);
            glm::vec3 topRightTile = glm::mix(leftEdgeTop, rightEdgeTop, u1);
            glm::vec3 bottomLeftTile = glm::mix(leftEdgeBottom, rightEdgeBottom, u0);
            glm::vec3 bottomRightTile = glm::mix(leftEdgeBottom, rightEdgeBottom, u1);

            // Create the tile
            makeTile(topLeftTile, topRightTile, bottomLeftTile, bottomRightTile);
        }
    }


}

void Cube::setVertexData() {
    // Uncomment these lines for Task 2, then comment them out for Task 3:

    makeTile(glm::vec3(-0.5f,  0.5f, 0.5f),
             glm::vec3( 0.5f,  0.5f, 0.5f),
             glm::vec3(-0.5f, -0.5f, 0.5f),
             glm::vec3( 0.5f, -0.5f, 0.5f));

    // Uncomment these lines for Task 3:

    makeFace(glm::vec3(-0.5f,  0.5f, 0.5f),
             glm::vec3( 0.5f,  0.5f, 0.5f),
             glm::vec3(-0.5f, -0.5f, 0.5f),
             glm::vec3( 0.5f, -0.5f, 0.5f));

    // Task 4: Use the makeFace() function to make all 6 sides of the cube
    glm::vec3 p0(-0.5f,  0.5f,  0.5f); // Top-left-front
    glm::vec3 p1( 0.5f,  0.5f,  0.5f); // Top-right-front
    glm::vec3 p2(-0.5f, -0.5f,  0.5f); // Bottom-left-front
    glm::vec3 p3( 0.5f, -0.5f,  0.5f); // Bottom-right-front

    glm::vec3 p4(-0.5f,  0.5f, -0.5f); // Top-left-back
    glm::vec3 p5( 0.5f,  0.5f, -0.5f); // Top-right-back
    glm::vec3 p6(-0.5f, -0.5f, -0.5f); // Bottom-left-back
    glm::vec3 p7( 0.5f, -0.5f, -0.5f); // Bottom-right-back

    // Front face (Z = 0.5)
    makeFace(p0, p1, p2, p3);

    // Back face (Z = -0.5)
    makeFace(p5, p4, p7, p6);

    // Left face (X = -0.5)
    makeFace(p4, p0, p6, p2);

    // Right face (X = 0.5)
    makeFace(p1, p5, p3, p7);

    // Top face (Y = 0.5)
    makeFace(p4, p5, p0, p1);

    // Bottom face (Y = -0.5)
    makeFace(p2, p3, p6, p7);
}

// Inserts a glm::vec3 into a vector of floats.
// This will come in handy if you want to take advantage of vectors to build your shape!
void Cube::insertVec3(std::vector<float> &data, glm::vec3 v) {
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
}

