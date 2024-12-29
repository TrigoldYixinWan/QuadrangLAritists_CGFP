#include "realtime.h"
#include "glm/gtc/type_ptr.hpp"

#include "utils/shaderloader.h"
#include <QCoreApplication>
#include <QMouseEvent>
#include <QKeyEvent>
#include <iostream>
#include "settings.h"
#include <glm/gtx/string_cast.hpp>
#include <Box2D/Box2D.h>

// ================== Project 5: Lights, Camera
QString texturePaths[7] = {
    ":/resources/planetText/venus.png",
    ":/resources/planetText/earth.png",
    ":/resources/planetText/mars.png",
    ":/resources/planetText/jupiter.png",
    ":/resources/planetText/saturn.png",
    ":/resources/planetText/uranus.png",
    ":/resources/planetText/neptune.png"
};

Realtime::Realtime(QWidget *parent)
    : QOpenGLWidget(parent)
{
    m_prev_mouse_pos = glm::vec2(size().width()/2, size().height()/2);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    m_keyMap[Qt::Key_W]       = false;
    m_keyMap[Qt::Key_A]       = false;
    m_keyMap[Qt::Key_S]       = false;
    m_keyMap[Qt::Key_D]       = false;
    m_keyMap[Qt::Key_Control] = false;
    m_keyMap[Qt::Key_Space]   = false;

    // Start the timer for updates (assuming 60 FPS)
    m_timer = startTimer(16); // roughly every 16 ms
    m_elapsedTimer.start();

    // If you must use this function, do not edit anything above this
}

void Realtime::finish() {
    // Stop the timer
    killTimer(m_timer);
    makeCurrent();

    // Delete OpenGL resources
    for (const auto& shapeData : m_renderData.shapes) {
        if (shapeData.VAO) {
            glDeleteVertexArrays(1, &shapeData.VAO);
            glDeleteBuffers(1, &shapeData.VBO);
            if (shapeData.EBO) {
                glDeleteBuffers(1, &shapeData.EBO);
            }
        }
    }

    // Delete shader programs
    glDeleteProgram(m_shaderProgram);
    glDeleteProgram(m_textureShader);

    // Delete FBO resources
    glDeleteFramebuffers(1, &m_fbo);
    glDeleteTextures(1, &m_fbo_texture);
    glDeleteRenderbuffers(1, &m_fbo_renderbuffer);

    // Delete fullscreen quad VAO and VBO
    glDeleteVertexArrays(1, &m_fullscreen_vao);
    glDeleteBuffers(1, &m_fullscreen_vbo);

    doneCurrent();
}

void Realtime::initializeGL() {
    glewExperimental = GL_TRUE;
    glewInit();

    glDisable(GL_DEPTH_TEST); // Not necessary in 2D
    glDisable(GL_CULL_FACE);  // Usually unnecessary for 2D

    glClearColor(0.1f, 0.1f, 0.1f, 1.f);

    // Setup a simple orthographic projection
    // For example, we can set the world coordinate system to something like:
    // worldWidth = width_in_world_units;
    // worldHeight = height_in_world_units;
    // A common approach: worldWidth = w/100.0f, worldHeight = h/100.0f to scale world vs. pixels
    m_screenWidth = width();
    m_screenHeight = height();
    m_worldWidth = 10.0f;    // Example: 10 world units wide
    m_worldHeight = 10.0f * (m_screenHeight / m_screenWidth); // maintain aspect ratio

    setup2DProjection(width(), height());

    // Initialize simple shader (vertex + fragment) for drawing 2D shapes
    // You can use a very basic shader:
    // Vertex shader: just pass through position
    // Fragment shader: output a solid color
    m_shaderProgram2D = ShaderLoader::createShaderProgram(
        ":/resources/shaders/2D.vert",
        ":/resources/shaders/2D.frag"
        );

    // Create Box2D world with gravity
    b2Vec2 gravity(0.0f, -9.8f);

    m_world = new b2World(gravity);

    // Create ground body
    {
        b2BodyDef groundDef;
        groundDef.position.Set(0.0f, -m_worldHeight / 2.0f - 1.0f);
        m_groundBody = m_world->CreateBody(&groundDef);

        b2PolygonShape groundBox;
        groundBox.SetAsBox(m_worldWidth, 1.0f);
        m_groundBody->CreateFixture(&groundBox, 0.0f);
    }
    {
        b2ParticleSystemDef particleSystemDef;
        particleSystemDef.radius = 0.05f; // Adjust for desired density
        particleSystemDef.dampingStrength = 0.2f;
        m_particleSystem = m_world->CreateParticleSystem(&particleSystemDef);
        m_particleSystem->SetGravityScale(1.0f);
        m_particleSystem->SetMaxParticleCount(5000); // Limit particle count
    }
    m_timer = startTimer(16); // ~60FPS
}
void Realtime::setup2DProjection(int w, int h) {
    // Compute aspect ratio and adjust your orthographic projection
    glViewport(0,0,w,h);
    m_screenWidth = w;
    m_screenHeight = h;
    m_worldHeight = m_worldWidth * (float(h)/float(w));

    // In a 2D shader, you'll just supply an orthographic matrix:
    // e.g. projection = glm::ortho(-m_worldWidth/2.0f, m_worldWidth/2.0f, -m_worldHeight/2.0f, m_worldHeight/2.0f, -1.0f, 1.0f);
    // Pass this uniform to your shaders every frame.
}
void Realtime::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(m_shaderProgram2D);

    glm::mat4 proj = glm::ortho(-m_worldWidth/2.0f, m_worldWidth/2.0f,
                                -m_worldHeight/2.0f, m_worldHeight/2.0f,
                                -1.0f, 1.0f);
    GLint projLoc = glGetUniformLocation(m_shaderProgram2D, "u_Projection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj));

    GLint colorLoc = glGetUniformLocation(m_shaderProgram2D, "u_Color");
    GLint modelLoc = glGetUniformLocation(m_shaderProgram2D, "u_Model");
    GLint planetTypeLoc = glGetUniformLocation(m_shaderProgram2D, "u_PlanetType");
    GLint lightPosLoc = glGetUniformLocation(m_shaderProgram2D, "u_LightPos");
    GLint timeLoc = glGetUniformLocation(m_shaderProgram2D, "u_Time");

    GLint useTextureLoc = glGetUniformLocation(m_shaderProgram2D, "u_UseTexture");
    GLint textureLoc = glGetUniformLocation(m_shaderProgram2D, "u_Texture");

    // Set light position (sun position, which is 0,0)
    glUniform2f(lightPosLoc, 0.0f, 0.0f);

    // Set time (use elapsed time since start)
    float currentTime = m_elapsedTimer.elapsed() / 1000.0f;
    glUniform1f(timeLoc, currentTime);

    int32 particleCount = m_particleSystem->GetParticleCount();
    if (particleCount > 0) {
        const b2Vec2* positions = m_particleSystem->GetPositionBuffer();

        // Initialize VAO/VBO once
        if (!m_particleVAOInitialized) {
            glGenVertexArrays(1, &m_particleVAO);
            glBindVertexArray(m_particleVAO);

            glGenBuffers(1, &m_particleVBO);
            glBindBuffer(GL_ARRAY_BUFFER, m_particleVBO);

            // Setup position attribute
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);

            // Setup texCoord attribute with default values
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
            glEnableVertexAttribArray(1);

            glBindVertexArray(0);
            m_particleVAOInitialized = true;
        }

        // Create a buffer that includes both position and default texture coordinates
        std::vector<float> vertexData;
        vertexData.reserve(particleCount * 4); // 4 floats per particle (2 for pos, 2 for texCoord)
        for (int i = 0; i < particleCount; i++) {
            // Position
            vertexData.push_back(positions[i].x);
            vertexData.push_back(positions[i].y);
            // Default texture coordinates
            vertexData.push_back(0.0f);
            vertexData.push_back(0.0f);
        }

        // Update particle positions into VBO
        glBindBuffer(GL_ARRAY_BUFFER, m_particleVBO);
        glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_DYNAMIC_DRAW);

        // Set uniforms and draw
        glm::mat4 particleModel = glm::mat4(1.0f);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(particleModel));
        glUniform3f(colorLoc, 0.0f, 0.0f, 1.0f);  // Blue color for water

        glBindVertexArray(m_particleVAO);
        glPointSize(5.0f);
        glUniform1i(planetTypeLoc, 99); // or some value that corresponds to no masking

        glDrawArrays(GL_POINTS, 0, particleCount);
        glBindVertexArray(0);
    }
    for (auto &obj : m_objects) {
        b2Vec2 pos = obj.body->GetPosition();
        float angle = obj.body->GetAngle();

        glm::mat4 model = glm::translate(glm::mat4(1.f), glm::vec3(pos.x, pos.y, 0.f));
        model = glm::rotate(model, angle, glm::vec3(0.f, 0.f, 1.f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        if (obj.hasTexture && obj.textureID != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, obj.textureID);
            glUniform1i(textureLoc, 0);
            glUniform1i(useTextureLoc, 1);
        } else {
            glUniform3fv(colorLoc, 1, glm::value_ptr(obj.color));
            glUniform1i(useTextureLoc, 0);
        }

        // // Set the object's color
        // glUniform3fv(colorLoc, 1, glm::value_ptr(obj.color));

        glBindVertexArray(obj.VAO);
        if (obj.isCircle) {
            // If it's a circle approximated by a triangle fan, draw with GL_TRIANGLE_FAN
            // The first vertex is the center, and then the other vertices form the fan
            // For a circle: num vertices = NUM_SEGMENTS + 1 (including center)
            glDrawArrays(GL_TRIANGLE_FAN, 0, 27);
        } else {
            // Box is 6 vertices forming 2 triangles
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        glBindVertexArray(0);
    }

    glUniform1i(planetTypeLoc, 99);
    renderBrushStrokes();

    glUseProgram(0);

}
void Realtime::resizeGL(int w, int h) {
    setup2DProjection(w, h);
    update();
}
void Realtime::sceneChanged() {
    clearScene();
    makeCurrent();

    // Parse the scene file
    if (!SceneParser::parse(settings.sceneFilePath, m_renderData)) {
        std::cerr << "Failed to parse scene file: " << settings.sceneFilePath << std::endl;
        doneCurrent();
        return;
    }



    // Initialize shapes (set up VAOs/VBOs)

    // Initialize lights

    doneCurrent();
    update(); // Request a repaint
}




void Realtime::settingsChanged() {
    // Update near and far plane distances
    m_camera.nearPlane = settings.nearPlane;
    m_camera.farPlane = settings.farPlane;
    m_camera.updateProjectionMatrix(width(), height());

    // Check if tessellation parameters have changed
    m_explosionStrength = settings.shapeParameter1;
    static int prevParam2 = settings.shapeParameter2;

    if (settings.shapeParameter1 != m_explosionStrength || settings.shapeParameter2 != prevParam2) {
        m_explosionStrength = settings.shapeParameter1;
        prevParam2 = settings.shapeParameter2;

        // Regenerate shapes with new tessellation parameters
        makeCurrent();
 // Re-initialize shapes with updated parameters
        doneCurrent();
    }

    update(); // Request a repaint
}




void Realtime::createPhysicsObject(float x, float y) {
    // Make the OpenGL context current before creating VAOs and VBOs
    makeCurrent();

    b2BodyDef bodyDef;
    bodyDef.type = b2_dynamicBody;
    bodyDef.position.Set(x, y);
    b2Body* body = m_world->CreateBody(&bodyDef);

    PhysObject obj;
    obj.body = body;
    obj.shape = m_currentShape;
    obj.color = m_currentColor;
    float halfSize = m_currentSize;

    if (obj.shape == ObjectShape::BOX) {
        obj.isCircle = false;
        obj.size = glm::vec2(halfSize);

        b2PolygonShape dynamicBox;
        dynamicBox.SetAsBox(halfSize, halfSize);

        b2FixtureDef fixtureDef;
        fixtureDef.shape = &dynamicBox;
        fixtureDef.density = 1.0f;
        fixtureDef.friction = 0.3f;
        body->CreateFixture(&fixtureDef);

        GLfloat verts[] = {
            -halfSize, -halfSize,
            halfSize, -halfSize,
            halfSize,  halfSize,

            -halfSize, -halfSize,
            halfSize,  halfSize,
            -halfSize,  halfSize
        };

        glGenVertexArrays(1, &obj.VAO);
        glBindVertexArray(obj.VAO);

        glGenBuffers(1, &obj.VBO);
        glBindBuffer(GL_ARRAY_BUFFER, obj.VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);

    } else if (obj.shape == ObjectShape::CIRCLE) {
        obj.isCircle = true;
        obj.size = glm::vec2(halfSize);

        b2CircleShape circle;
        circle.m_radius = halfSize;

        b2FixtureDef fixtureDef;
        fixtureDef.shape = &circle;
        fixtureDef.density = 1.0f;
        fixtureDef.friction = 0.3f;
        body->CreateFixture(&fixtureDef);

        const int NUM_SEGMENTS = 24;
        std::vector<GLfloat> circleVerts;

        // Center vertex with texture coordinates
        circleVerts.push_back(0.0f);  // x
        circleVerts.push_back(0.0f);  // y
        circleVerts.push_back(0.5f);  // s
        circleVerts.push_back(0.5f);  // t

        for (int i = 0; i <= NUM_SEGMENTS; i++) {
            float angle = (float)i / (float)NUM_SEGMENTS * 2.0f * M_PI;
            float xPos = halfSize * cos(angle);
            float yPos = halfSize * sin(angle);
            float s = 0.5f + (cos(angle) * 0.5f);  // Map to [0,1] range
            float t = 0.5f + (sin(angle) * 0.5f);  // Map to [0,1] range

            circleVerts.push_back(xPos);  // x
            circleVerts.push_back(yPos);  // y
            circleVerts.push_back(s);     // s
            circleVerts.push_back(t);     // t
        }

        glGenVertexArrays(1, &obj.VAO);
        glBindVertexArray(obj.VAO);

        glGenBuffers(1, &obj.VBO);
        glBindBuffer(GL_ARRAY_BUFFER, obj.VBO);
        glBufferData(GL_ARRAY_BUFFER, circleVerts.size() * sizeof(GLfloat), circleVerts.data(), GL_STATIC_DRAW);

        // Position attribute (2 floats)
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
        glEnableVertexAttribArray(0);

        // Texture coordinate attribute (2 floats)
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }

    m_objects.push_back(obj);

    // After finishing creation, you can let paintGL handle the rest.
    // No need to call doneCurrent() here, as QOpenGLWidget will handle context switching appropriately.
}

// ================== Project 6: Action!
void Realtime::keyPressEvent(QKeyEvent *event) {

    m_keyMap[Qt::Key(event->key())] = true;

    switch(event->key()) {
    case Qt::Key_1:
        m_currentShape = ObjectShape::BOX;
        break;
    case Qt::Key_2:
        m_currentShape = ObjectShape::CIRCLE;
        break;
    case Qt::Key_3:
        // Enter mode to select a new gravity center on next left click
        m_selectingGravityCenter = true;
        m_selectingExplosionCenter = false;
        m_selectingOrbitCenter = false;
        break;
    case Qt::Key_4:
        m_selectingExplosionCenter = true;
        m_selectingGravityCenter = false;
        m_selectingOrbitCenter = false;
        std::cout << "Explosion mode activated. Click on the screen to apply outward force." << std::endl;
        break;
    case Qt::Key_5:
        m_selectingOrbitCenter = true;
        m_selectingGravityCenter = false;
        m_selectingExplosionCenter = false;
        m_world->SetGravity(b2Vec2(0.0f, 0.0f));
        std::cout << "Orbit mode activated. Click on the screen to make objects orbit." << std::endl;
        break;
    case Qt::Key_6:
        // Initialize and activate the solar system scene
        initializeSolarSystem();
        break;
    case Qt::Key_E:
        m_currentSize += 0.1f;
        break;
    case Qt::Key_Q:
        m_currentSize = std::max(0.1f, m_currentSize - 0.1f);
        break;
    case Qt::Key_R:
        m_currentColor = glm::vec3(1.0f, 0.0f, 0.0f); // Red
        break;
    case Qt::Key_G:
        m_currentColor = glm::vec3(0.0f, 1.0f, 0.0f); // Green
        break;
    case Qt::Key_B:
        m_currentColor = glm::vec3(0.0f, 0.0f, 1.0f); // Blue
        break;
    case Qt::Key_Escape:
        resetGravityCenter();
        break;
    case Qt::Key_W:
        m_currentShape = ObjectShape::WATER;
        break;
    case Qt::Key_L:
        m_brushMode = !m_brushMode;

        break;
    case Qt::Key_0:
        resetWorld();
        break;

    default:
        break;
    }
}
GLuint loadTexture(const QString &filePath) {
    QImage img;
    if(!img.load(filePath)) {
        std::cerr << "Failed to load texture: " << filePath.toStdString() << std::endl;
        return 0;
    }
    img = img.convertToFormat(QImage::Format_RGBA8888);

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width(), img.height(),
                 0, GL_RGBA, GL_UNSIGNED_BYTE, img.constBits());

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texID;
}
void Realtime::resetWorld() {
    // Delete all Box2D bodies and reset vectors
    for (auto& obj : m_objects) {
        m_world->DestroyBody(obj.body);
    }
    m_objects.clear();

    // Clear all particles
    if (m_particleSystem) {
        m_particleSystem->DestroyParticle(0, false);
    }

    // Clear all brush strokes (both visual and physical)
    m_allBrushStrokes.clear();
    m_currentStroke.clear();

    // Destroy all brush bodies
    b2Body* body = m_world->GetBodyList();
    while (body) {
        b2Body* nextBody = body->GetNext(); // Get next before destroying current
        if (body != m_groundBody) { // Don't destroy ground yet
            m_world->DestroyBody(body);
        }
        body = nextBody;
    }

    // Reset current brush pointer
    m_currentBrush = nullptr;

    // Reset gravity and modes
    m_world->SetGravity(b2Vec2(0.0f, -9.8f));
    m_hasGravityCenter = false;
    m_orbitMode = false;
    m_brushMode = false;
    setCursor(Qt::ArrowCursor);

    // Recreate ground
    if (m_groundBody) {
        m_world->DestroyBody(m_groundBody);
    }
    b2BodyDef groundDef;
    groundDef.position.Set(0.0f, -m_worldHeight / 2.0f - 1.0f);
    m_groundBody = m_world->CreateBody(&groundDef);

    b2PolygonShape groundBox;
    groundBox.SetAsBox(m_worldWidth, 1.0f);
    m_groundBody->CreateFixture(&groundBox, 0.0f);

    update();
}
void Realtime::keyReleaseEvent(QKeyEvent *event) {
    m_keyMap[Qt::Key(event->key())] = false;
}

void Realtime::mousePressEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) {
        float xRatio = (float)event->pos().x() / m_screenWidth;
        float yRatio = (float)event->pos().y() / m_screenHeight;
        float worldX = (xRatio - 0.5f) * m_worldWidth;
        float worldY = (0.5f - yRatio) * m_worldHeight;

        if (settings.perPixelFilter) {
            // Set new gravity center
            m_gravityCenter = glm::vec2(worldX, worldY);
            m_hasGravityCenter = true;
            m_selectingGravityCenter = false;
        }if (m_brushMode) {
            float x = ((float)event->pos().x() / width() - 0.5f) * m_worldWidth;
            float y = (0.5f - (float)event->pos().y() / height()) * m_worldHeight;

            // Start new stroke
            m_currentStroke.clear();
            m_currentStroke.push_back(b2Vec2(x, y));
            m_mouseDown = true;

            // Create static body for the brush stroke
            b2BodyDef bodyDef;
            bodyDef.type = b2_staticBody;
            m_currentBrush = m_world->CreateBody(&bodyDef);
        }

        else if (settings.extraCredit2) {
            m_explosionCenter = glm::vec2(worldX, worldY);
            m_explosionMode = true;
            m_selectingExplosionCenter = false;
            m_hasGravityCenter = false;
        }

        else if (m_selectingOrbitCenter) {
            m_orbitCenter = glm::vec2(0.f);
            m_orbitMode = true;
            m_selectingOrbitCenter = false;
            m_hasGravityCenter = false;
            if (m_groundBody) {
                m_world->DestroyBody(m_groundBody);
                m_groundBody = nullptr;
                std::cout << "Ground body destroyed for orbit mode." << std::endl;
            }
        }else if (m_currentShape == ObjectShape::WATER) {
            makeCurrent();
            // Define a box of particles at clicked position
            b2PolygonShape particleBox;
            float halfSize = 0.5f;
            particleBox.SetAsBox(halfSize, halfSize, b2Vec2(worldX, worldY), 0);

            b2ParticleGroupDef groupDef;
            groupDef.shape = &particleBox;
            groupDef.flags = b2_waterParticle; // Water-like particles
            groupDef.color.Set(0, 0, 155, 255); // Blue color (only if rendering particle color)
            m_particleSystem->CreateParticleGroup(groupDef);
        }
        
        else {
            // Create a new physics object
            createPhysicsObject(worldX, worldY);
        }
    }
}

void Realtime::mouseReleaseEvent(QMouseEvent *event) {
    if (!m_brushMode) return;

    // Save completed stroke
    if (!m_currentStroke.empty()) {
        m_allBrushStrokes.push_back(m_currentStroke);
    }
    m_currentBrush = nullptr;
    m_mouseDown = false;

}

void Realtime::timerEvent(QTimerEvent *event) {
    float timeStep = 1.0f / 60.0f;
    int32 velocityIterations = 6;
    int32 positionIterations = 2;

    m_world->Step(timeStep, velocityIterations, positionIterations);

    if (m_hasGravityCenter) {
        // Apply radial gravity toward m_gravityCenter
        for (auto &obj : m_objects) {
            b2Body* body = obj.body;
            if (body->GetType() == b2_dynamicBody) {
                b2Vec2 bodyPos = body->GetPosition();
                glm::vec2 dir = glm::vec2(m_gravityCenter.x - bodyPos.x, m_gravityCenter.y - bodyPos.y);
                float distSq = dir.x * dir.x + dir.y * dir.y;
                if (distSq > 0.0001f) {
                    float dist = sqrt(distSq);
                    // Normalized direction
                    glm::vec2 norm = dir / dist;

                    float mass = body->GetMass();
                    glm::vec2 force = norm * (m_gravityStrength * mass);

                    body->ApplyForceToCenter(b2Vec2(force.x *2, force.y*2), true);
                }
            }
        }
    }

    if (m_explosionMode) {
        // Apply outward force from the click position
        // glm::vec2 explosionCenter(worldX, worldY);
        float explosionStrength = m_explosionStrength; // Adjust the strength of the explosion

        for (auto &obj : m_objects) {
            b2Body* body = obj.body;
            if (body->GetType() == b2_dynamicBody) {
                b2Vec2 bodyPos = body->GetPosition();
                glm::vec2 direction = glm::vec2(bodyPos.x - m_explosionCenter.x, bodyPos.y - m_explosionCenter.y);

                float distanceSq = direction.x * direction.x + direction.y * direction.y;

                // Avoid division by zero and apply force only within a certain range
                if (distanceSq > 0.0001f && distanceSq < 10.0f) {
                    float distance = sqrt(distanceSq);
                    glm::vec2 normalizedDirection = direction / distance;

                    // Scale force by explosion strength and inverse distance
                    glm::vec2 force = normalizedDirection * (explosionStrength / distance);

                    body->ApplyForceToCenter(b2Vec2(force.x, force.y), true);
                }
            }
        }
        m_explosionMode = false;
    }

        if (m_orbitMode) {
            for (auto &obj : m_objects) {
                b2Body* body = obj.body;
                if (body->GetType() == b2_dynamicBody) {
                    b2Vec2 bodyPos = body->GetPosition();
                    glm::vec2 pos(bodyPos.x, bodyPos.y);
                    glm::vec2 diff = pos - m_orbitCenter;
                    float dist = glm::length(diff);

                    if (dist < 0.0001f) {
                        continue;
                    }

                    glm::vec2 radialDir = diff / dist;
                    glm::vec2 tangentialDir(-radialDir.y, radialDir.x);

                    float angularSpeed = obj.orbitAngularSpeed;
                    float desiredSpeed = (0.8+settings.shapeParameter2/5)*angularSpeed * dist; // v = ω * r

                    b2Vec2 bVel = body->GetLinearVelocity();
                    glm::vec2 vel(bVel.x, bVel.y);

                    float tangentialComponent = glm::dot(vel, tangentialDir);
                    float speedError = desiredSpeed - tangentialComponent;

                    float tangentForceGain = 10.0f;
                    glm::vec2 tangentForce = tangentialDir * speedError * tangentForceGain * body->GetMass();
                    body->ApplyForceToCenter(b2Vec2(tangentForce.x, tangentForce.y), true);

                    float centripetalForceMagnitude = (desiredSpeed * desiredSpeed / dist) * body->GetMass();
                    glm::vec2 centripetalForce = -radialDir * centripetalForceMagnitude;
                    body->ApplyForceToCenter(b2Vec2(centripetalForce.x, centripetalForce.y), true);
                }
            }
        }



    update(); // request a repaint
}

void Realtime::mouseMoveEvent(QMouseEvent *event) {
    if (!m_brushMode || !m_currentBrush || !m_mouseDown) return;

    float x = ((float)event->pos().x() / width() - 0.5f) * m_worldWidth;
    float y = (0.5f - (float)event->pos().y() / height()) * m_worldHeight;

    b2Vec2 newPoint(x, y);
    if (!m_currentStroke.empty()) {
        b2Vec2 lastPoint = m_currentStroke.back();
        float dist = b2Distance(newPoint, lastPoint);
        if (dist < m_brushThickness) return;
    }

    m_currentStroke.push_back(newPoint);

    // Create edge shape between last two points
    if (m_currentStroke.size() >= 2 ) {
        size_t last = m_currentStroke.size() - 1;

        b2EdgeShape edge;
        edge.Set(m_currentStroke[last-1], m_currentStroke[last]);

        b2FixtureDef fixtureDef;
        fixtureDef.shape = &edge;
        fixtureDef.density = 0.0f;  // Static body
        fixtureDef.friction = 0.3f;

        m_currentBrush->CreateFixture(&fixtureDef);
    }

    update();
}


// DO NOT EDIT
void Realtime::saveViewportImage(std::string filePath) {
    // Make sure we have the right context and everything has been drawn
    makeCurrent();

    int fixedWidth = 1024;
    int fixedHeight = 768;

    // Create Frame Buffer
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // Create a color attachment texture
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, fixedWidth, fixedHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    // Optional: Create a depth buffer if your rendering uses depth testing
    GLuint rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, fixedWidth, fixedHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Error: Framebuffer is not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return;
    }

    // Render to the FBO
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, fixedWidth, fixedHeight);

    // Clear and render your scene here
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    paintGL();

    // Read pixels from framebuffer
    std::vector<unsigned char> pixels(fixedWidth * fixedHeight * 3);
    glReadPixels(0, 0, fixedWidth, fixedHeight, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    // Unbind the framebuffer to return to default rendering to the screen
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Convert to QImage
    QImage image(pixels.data(), fixedWidth, fixedHeight, QImage::Format_RGB888);
    QImage flippedImage = image.mirrored(); // Flip the image vertically

    // Save to file using Qt
    QString qFilePath = QString::fromStdString(filePath);
    if (!flippedImage.save(qFilePath)) {
        std::cerr << "Failed to save image to " << filePath << std::endl;
    }

    // Clean up
    glDeleteTextures(1, &texture);
    glDeleteRenderbuffers(1, &rbo);
    glDeleteFramebuffers(1, &fbo);
}





void Realtime::clearScene() {
    // Ensure the OpenGL context is current
    makeCurrent();

    // Delete VAOs, VBOs, and EBOs associated with the shapes
    for (auto& shapeData : m_renderData.shapes) {
        if (shapeData.VAO) {
            glDeleteVertexArrays(1, &shapeData.VAO);
            shapeData.VAO = 0;
        }
        if (shapeData.VBO) {
            glDeleteBuffers(1, &shapeData.VBO);
            shapeData.VBO = 0;
        }
        if (shapeData.EBO) {
            glDeleteBuffers(1, &shapeData.EBO);
            shapeData.EBO = 0;
        }
    }

    // Clear the shapes vector
    m_renderData.shapes.clear();

    // Clear lights and other scene data if necessary
    m_renderData.lights.clear();

    // If you have textures or other resources, delete them here

    doneCurrent();
}

void Realtime::resetGravityCenter() {
    m_gravityCenter = glm::vec2(0.0f);
    m_hasGravityCenter = false;
    m_selectingGravityCenter = false;
    m_orbitMode = false;
    m_selectingOrbitCenter = false;
    m_world->SetGravity(b2Vec2(0.0f, -9.8f));

    if (!m_groundBody) {
        b2BodyDef groundDef;
        groundDef.position.Set(0.0f, -m_worldHeight / 2.0f - 1.0f);
        m_groundBody = m_world->CreateBody(&groundDef);

        b2PolygonShape groundBox;
        groundBox.SetAsBox(m_worldWidth, 1.0f);
        m_groundBody->CreateFixture(&groundBox, 0.0f);
    }
    
    std::cout << "Gravity center reset." << std::endl;

    update();
}

void Realtime::initializeSolarSystem() {
    // Clear and destroy existing objects
    for (auto &obj : m_objects) {
        m_world->DestroyBody(obj.body);
    }
    m_objects.clear();

    if (m_groundBody) {
        m_world->DestroyBody(m_groundBody);
        m_groundBody = nullptr;
    }

    // Set gravity to zero and enable orbit mode
    m_world->SetGravity(b2Vec2(0.0f, 0.0f));
    m_orbitCenter = glm::vec2(0.0f, 0.0f);
    m_orbitMode = true;

    // Create the sun
    {
        b2BodyDef sunDef;
        sunDef.type = b2_staticBody;
        sunDef.position.Set(0.0f, 0.0f);
        b2Body* sunBody = m_world->CreateBody(&sunDef);

        b2CircleShape sunShape;
        sunShape.m_radius = 0.25f;
        b2FixtureDef fixtureDef;
        fixtureDef.shape = &sunShape;
        fixtureDef.density = 0.0f;
        fixtureDef.friction = 0.0f;
        sunBody->CreateFixture(&fixtureDef);

        PhysObject sunObj;
        sunObj.body = sunBody;
        sunObj.shape = ObjectShape::CIRCLE;
        sunObj.color = glm::vec3(1.0f, 1.0f, 0.0f);
        float halfSize = 0.25f;

        const int NUM_SEGMENTS = 24;
        std::vector<GLfloat> circleVerts;
        circleVerts.push_back(0.0f);
        circleVerts.push_back(0.0f);
        for (int i = 0; i <= NUM_SEGMENTS; i++) {
            float angle = (float)i / (float)NUM_SEGMENTS * 2.0f * M_PI;
            float xPos = halfSize * cos(angle);
            float yPos = halfSize * sin(angle);
            circleVerts.push_back(xPos);
            circleVerts.push_back(yPos);
        }

        glGenVertexArrays(1, &sunObj.VAO);
        glBindVertexArray(sunObj.VAO);

        glGenBuffers(1, &sunObj.VBO);
        glBindBuffer(GL_ARRAY_BUFFER, sunObj.VBO);
        glBufferData(GL_ARRAY_BUFFER, circleVerts.size() * sizeof(GLfloat), circleVerts.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
        sunObj.isCircle = true;
        sunObj.size = glm::vec2(halfSize);

        QString sunTexturePath = ":/resources/planetText/test.png";
        GLuint sunTexID = loadTexture(sunTexturePath);
        if (sunTexID != 0) {
            sunObj.textureID = sunTexID;
            sunObj.hasTexture = true;
        } else {
            sunObj.hasTexture = false;
        }

        m_objects.push_back(sunObj);
    }

    // Assign realistic(ish) angular speeds (in rad/s) to planets:
    // Example using 1 simulated year = 60 seconds scaling:
    // Mercury, Venus, Earth, Mars, Jupiter, Saturn, Uranus
    std::vector<float> angularSpeeds = {
        0.4345f,  // Mercury
        0.1700f,  // Venus
        0.1047f,  // Earth
        0.0557f,  // Mars
        0.00883f, // Jupiter
        0.00355f, // Saturn
        0.001247f // Uranus
    };

    m_currentShape = ObjectShape::CIRCLE;
    m_currentSize = 0.1f;
    float baseRadius = 1.5f;
    for (int i = 0; i < 7; i++) {
        float radius = baseRadius + i * 0.5f;
        createPhysicsObject(radius, 0.0f);
        if (!m_objects.empty()) {
            float hue = (float)i / 7.0f;
            m_objects.back().color = glm::vec3(hue, 0.5f, 1.0f - hue);
            m_objects.back().orbitAngularSpeed = angularSpeeds[i];

            GLuint texID = loadTexture(texturePaths[i]);
            if (texID != 0) {
                m_objects.back().textureID = texID;
                m_objects.back().hasTexture = true;
            } else {
                m_objects.back().hasTexture = false;
            }
        }
    }
}


void Realtime::renderBrushStrokes() {
    glUseProgram(m_shaderProgram2D);

    glm::mat4 proj = glm::ortho(-m_worldWidth/2.0f, m_worldWidth/2.0f,
                                -m_worldHeight/2.0f, m_worldHeight/2.0f,
                                -1.0f, 1.0f);
    GLint projLoc = glGetUniformLocation(m_shaderProgram2D, "u_Projection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj));

    GLint colorLoc = glGetUniformLocation(m_shaderProgram2D, "u_Color");
    GLint modelLoc = glGetUniformLocation(m_shaderProgram2D, "u_Model");

    // Set color and model matrix
    glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);  // White color
    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glLineWidth(10.f);  // Make lines thicker and visible

    // Render all completed strokes
    for (const auto& stroke : m_allBrushStrokes) {
        if (stroke.size() >= 2) {
            std::vector<float> vertices;
            for (const auto& point : stroke) {
                vertices.push_back(point.x);
                vertices.push_back(point.y);
            }

            GLuint vbo, vao;
            glGenVertexArrays(1, &vao);
            glGenBuffers(1, &vbo);

            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
                         vertices.data(), GL_STATIC_DRAW);

            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(0);

            glDrawArrays(GL_LINE_STRIP, 0, vertices.size()/2);

            glDeleteBuffers(1, &vbo);
            glDeleteVertexArrays(1, &vao);
        }
    }

    // Render current stroke if it exists
    if (m_currentStroke.size() >= 2) {
        std::vector<float> vertices;
        for (const auto& point : m_currentStroke) {
            vertices.push_back(point.x);
            vertices.push_back(point.y);
        }

        GLuint vbo, vao;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
                     vertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);

        glDrawArrays(GL_LINE_STRIP, 0, vertices.size()/2);

        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
    }

    glUseProgram(0);
}



















































