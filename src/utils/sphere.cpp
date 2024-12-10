#include "Sphere.h"
#include <glm/glm.hpp>
#include <algorithm>
#include "realtime.h"
#include "glm/gtc/type_ptr.hpp"
#include "utils/cube.h"
#include "utils/sphere.h"
#include "utils/cone.h"
#include "utils/cylinder.h"

#include "utils/shaderloader.h"
#include <QCoreApplication>
#include <QMouseEvent>
#include <QKeyEvent>
#include <iostream>
#include "settings.h"
#include <glm/gtx/string_cast.hpp>
#include <Box2D/Box2D.h>

void Sphere::updateParams(int param1, int param2) {
    m_vertexData.clear();
    m_param1 = std::max(3, param1); // Ensure at least 3 segments to form a polygon
    m_param2 = param2; // Might not be used for a simple 2D circle
    setVertexData();
}

void Sphere::setVertexData() {
    m_vertexData.clear();
    // We'll create a circle of radius 0.5f centered at (0,0)
    float r = 0.5f;
    int numSegments = m_param1;
    float angleStep = 2.0f * glm::pi<float>() / numSegments;

    // Insert center of the circle
    insertVec2(m_vertexData, glm::vec2(0.0f, 0.0f));

    // Insert the circle vertices around the center
    for (int i = 0; i <= numSegments; i++) {
        float angle = i * angleStep;
        float x = r * cos(angle);
        float y = r * sin(angle);
        insertVec2(m_vertexData, glm::vec2(x, y));
    }
}

// For a 2D shape, we only need to insert vec2 positions.
// No normals are required in a 2D pipeline, unless you have another reason to store them.

