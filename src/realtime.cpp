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

// ================== Project 5: Lights, Camera

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

    glClearColor(0.5f, 0.5f, 0.5f, 1.f);

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
        groundDef.position.Set(0.0f, -m_worldHeight / 2.0f + 1.0f);
        b2Body* groundBody = m_world->CreateBody(&groundDef);

        b2PolygonShape groundBox;
        groundBox.SetAsBox(m_worldWidth, 1.0f);
        groundBody->CreateFixture(&groundBox, 0.0f);
    }

    // Start timer for updates
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

    for (auto &obj : m_objects) {
        b2Vec2 pos = obj.body->GetPosition();
        float angle = obj.body->GetAngle();

        glm::mat4 model = glm::translate(glm::mat4(1.f), glm::vec3(pos.x, pos.y, 0.f));
        model = glm::rotate(model, angle, glm::vec3(0.f, 0.f, 1.f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        // Set the object's color
        glUniform3fv(colorLoc, 1, glm::value_ptr(obj.color));

        glBindVertexArray(obj.VAO);
        if (obj.isCircle) {
            // If it's a circle approximated by a triangle fan, draw with GL_TRIANGLE_FAN
            // The first vertex is the center, and then the other vertices form the fan
            // For a circle: num vertices = NUM_SEGMENTS + 1 (including center)
            glDrawArrays(GL_TRIANGLE_FAN, 0, 24 + 1);
        } else {
            // Box is 6 vertices forming 2 triangles
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        glBindVertexArray(0);
    }

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
    static int prevParam1 = settings.shapeParameter1;
    static int prevParam2 = settings.shapeParameter2;

    if (settings.shapeParameter1 != prevParam1 || settings.shapeParameter2 != prevParam2) {
        prevParam1 = settings.shapeParameter1;
        prevParam2 = settings.shapeParameter2;

        // Regenerate shapes with new tessellation parameters
        makeCurrent();
 // Re-initialize shapes with updated parameters
        doneCurrent();
    }

    update(); // Request a repaint
}

void Realtime::renderScene() {
    // Use the shader program
    glUseProgram(m_shaderProgram);

    // Get uniform locations
    GLint modelLoc = glGetUniformLocation(m_shaderProgram, "model");
    GLint viewLoc = glGetUniformLocation(m_shaderProgram, "view");
    GLint projLoc = glGetUniformLocation(m_shaderProgram, "proj");
    GLint cameraPosLoc = glGetUniformLocation(m_shaderProgram, "cameraPos");
    GLint numLightsLoc = glGetUniformLocation(m_shaderProgram, "numLights");

    // Material uniforms
    GLint materialAmbientLoc  = glGetUniformLocation(m_shaderProgram, "material.ambient");
    GLint materialDiffuseLoc  = glGetUniformLocation(m_shaderProgram, "material.diffuse");
    GLint materialSpecularLoc = glGetUniformLocation(m_shaderProgram, "material.specular");
    GLint materialShininessLoc = glGetUniformLocation(m_shaderProgram, "material.shininess");


    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(m_camera.viewMatrix));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(m_camera.projectionMatrix));

    // Set camera position
    glUniform3fv(cameraPosLoc, 1, glm::value_ptr(m_camera.position));

    // Set number of lights
    glUniform1i(numLightsLoc, lights.size());

    // For each light, set its properties
    for (int i = 0; i < lights.size(); ++i) {
        Light &light = lights[i];
        std::string baseName = "lights[" + std::to_string(i) + "]";

        GLint typeLoc = glGetUniformLocation(m_shaderProgram, (baseName + ".type").c_str());
        GLint posLoc = glGetUniformLocation(m_shaderProgram, (baseName + ".position").c_str());
        GLint dirLoc = glGetUniformLocation(m_shaderProgram, (baseName + ".direction").c_str());
        GLint colorLoc = glGetUniformLocation(m_shaderProgram, (baseName + ".color").c_str());
        GLint angleLoc = glGetUniformLocation(m_shaderProgram, (baseName + ".angle").c_str());
        GLint penumbraLoc = glGetUniformLocation(m_shaderProgram, (baseName + ".penumbra").c_str());
        GLint attenuationLoc = glGetUniformLocation(m_shaderProgram, (baseName + ".attenuation").c_str());

        glUniform1i(typeLoc, light.type);
        glUniform3fv(posLoc, 1, glm::value_ptr(light.position));
        glUniform3fv(dirLoc, 1, glm::value_ptr(light.direction));
        glUniform3fv(colorLoc, 1, glm::value_ptr(light.color));
        glUniform1f(angleLoc, light.angle);
        glUniform1f(penumbraLoc, light.penumbra);
        glUniform3fv(attenuationLoc, 1, glm::value_ptr(light.attenuation));
    }
    // Render each shape
    for (const auto& shapeData : m_renderData.shapes) {
        // Set model matrix for the shape
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(shapeData.ctm));

        // Set material properties per shape
        glUniform3fv(materialAmbientLoc, 1, glm::value_ptr(shapeData.primitive.material.cAmbient*m_renderData.globalData.ka));
        glUniform3fv(materialDiffuseLoc, 1, glm::value_ptr(shapeData.primitive.material.cDiffuse*m_renderData.globalData.kd));
        glUniform3fv(materialSpecularLoc, 1, glm::value_ptr(shapeData.primitive.material.cSpecular*m_renderData.globalData.ks));
        glUniform1f(materialShininessLoc, shapeData.primitive.material.shininess);

        // Bind VAO and draw the shape
        glBindVertexArray(shapeData.VAO);

        if (shapeData.hasIndices) {
            glDrawElements(GL_TRIANGLES, shapeData.indexCount, GL_UNSIGNED_INT, 0);
        } else {
            glDrawArrays(GL_TRIANGLES, 0, shapeData.vertexCount);
        }

        glBindVertexArray(0);
    }

    // Unbind the shader program
    glUseProgram(0);
}

void Realtime::paintTexture(GLuint texture, int filterType)
{
    glUseProgram(m_textureShader);

    // Set the filter type uniform
    GLint filterTypeLoc = glGetUniformLocation(m_textureShader, "u_filterType");
    glUniform1i(filterTypeLoc, filterType);

    // Set the texel size uniform
    GLint texelSizeLoc = glGetUniformLocation(m_textureShader, "u_texelSize");
    glUniform2f(texelSizeLoc, 1.0f / m_fbo_width, 1.0f / m_fbo_height);

    // Bind the fullscreen quad VAO
    glBindVertexArray(m_fullscreen_vao);

    // Bind the texture to texture unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    // Set the texture uniform to use texture unit 0
    GLint textureUniformLocation = glGetUniformLocation(m_textureShader, "u_texture");
    glUniform1i(textureUniformLocation, 0);

    // Draw the quad
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Unbind
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    glUseProgram(0);
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
        circleVerts.push_back(0.0f);
        circleVerts.push_back(0.0f);

        for (int i = 0; i <= NUM_SEGMENTS; i++) {
            float angle = (float)i / (float)NUM_SEGMENTS * 2.0f * M_PI;
            float xPos = halfSize * cos(angle);
            float yPos = halfSize * sin(angle);
            circleVerts.push_back(xPos);
            circleVerts.push_back(yPos);
        }

        glGenVertexArrays(1, &obj.VAO);
        glBindVertexArray(obj.VAO);

        glGenBuffers(1, &obj.VBO);
        glBindBuffer(GL_ARRAY_BUFFER, obj.VBO);
        glBufferData(GL_ARRAY_BUFFER, circleVerts.size() * sizeof(GLfloat), circleVerts.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(0);

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
        break;
    case Qt::Key_4:
        m_selectingExplosionCenter = true;
        m_selectingGravityCenter = false;
        std::cout << "Explosion mode activated. Click on the screen to apply outward force." << std::endl;
        break;
    case Qt::Key_Plus:
        m_currentSize += 0.1f;
        break;
    case Qt::Key_Minus:
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
    default:
        break;
    }
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

        if (m_selectingGravityCenter) {
            // Set new gravity center
            m_gravityCenter = glm::vec2(worldX, worldY);
            m_hasGravityCenter = true;
            m_selectingGravityCenter = false;
        } 

        else if (m_selectingExplosionCenter) {
            m_explosionCenter = glm::vec2(worldX, worldY);
            m_explosionMode = true;
            m_selectingExplosionCenter = false;
            m_hasGravityCenter = false;
        }
        
        else {
            // Create a new physics object
            createPhysicsObject(worldX, worldY);
        }
    }
}

void Realtime::mouseReleaseEvent(QMouseEvent *event) {
    if (!event->buttons().testFlag(Qt::LeftButton)) {
        m_mouseDown = false;
    }
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

                    // Gravity force = G * mass (you can scale by distance if you want realistic "orbital" gravity)
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
        float explosionStrength = 200.0f; // Adjust the strength of the explosion

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


    update(); // request a repaint
}

void Realtime::mouseMoveEvent(QMouseEvent *event) {
    if (m_mouseDown) {
        float sensitivity = 0.005f; // Adjust sensitivity as needed

        int posX = event->position().x();
        int posY = event->position().y();

        float deltaX = posX - m_prev_mouse_pos.x;
        float deltaY = posY - m_prev_mouse_pos.y;
        m_prev_mouse_pos = glm::vec2(posX, posY);

        // Convert pixel movement to angles
        float angleX = -deltaX * sensitivity;
        float angleY = deltaY * sensitivity;

        // Rotate around world up vector
        rotateAroundWorldUp(angleX);

        // Rotate around camera's right vector
        rotateAroundCameraRight(-angleY); // Negative to invert Y-axis if desired

        // Update the view matrix
        m_camera.updateViewMatrix();

        update(); // Request a repaint
    }
}

void Realtime::rotateAroundWorldUp(float angle) {
    // Create rotation matrix around world up vector (0, 1, 0)
    glm::mat4 rotationMatrix = createRotationMatrix(glm::vec3(0.0f, 1.0f, 0.0f), angle);

    // Rotate the look vector
    m_camera.look = glm::vec3(rotationMatrix * glm::vec4(m_camera.look, 0.0f));

    // Re-normalize the look vector
    m_camera.look = glm::normalize(m_camera.look);

    // Update the up vector
    m_camera.up = glm::vec3(rotationMatrix * glm::vec4(m_camera.up, 0.0f));
    m_camera.up = glm::normalize(m_camera.up);
}

void Realtime::rotateAroundCameraRight(float angle) {
    // Calculate the right vector
    glm::vec3 right = glm::normalize(glm::cross(m_camera.look, m_camera.up));

    // Create rotation matrix around the right vector
    glm::mat4 rotationMatrix = createRotationMatrix(right, angle);

    // Rotate the look vector
    m_camera.look = glm::vec3(rotationMatrix * glm::vec4(m_camera.look, 0.0f));
    m_camera.look = glm::normalize(m_camera.look);

    // Rotate the up vector
    m_camera.up = glm::vec3(rotationMatrix * glm::vec4(m_camera.up, 0.0f));
    m_camera.up = glm::normalize(m_camera.up);
}

glm::mat4 Realtime::createRotationMatrix(const glm::vec3 &axis, float angle) {
    // Normalize the axis
    glm::vec3 u = glm::normalize(axis);

    float cosTheta = cos(angle);
    float sinTheta = sin(angle);
    float oneMinusCosTheta = 1.0f - cosTheta;

    // Components for the rotation matrix
    float ux = u.x;
    float uy = u.y;
    float uz = u.z;

    // Construct the rotation matrix manually
    glm::mat4 rotationMatrix = glm::mat4(
        cosTheta + ux * ux * oneMinusCosTheta,
        uy * ux * oneMinusCosTheta + uz * sinTheta,
        uz * ux * oneMinusCosTheta - uy * sinTheta,
        0.0f,

        ux * uy * oneMinusCosTheta - uz * sinTheta,
        cosTheta + uy * uy * oneMinusCosTheta,
        uz * uy * oneMinusCosTheta + ux * sinTheta,
        0.0f,

        ux * uz * oneMinusCosTheta + uy * sinTheta,
        uy * uz * oneMinusCosTheta - ux * sinTheta,
        cosTheta + uz * uz * oneMinusCosTheta,
        0.0f,

        0.0f, 0.0f, 0.0f, 1.0f
        );

    return rotationMatrix;
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

    std::cout << "Gravity center reset." << std::endl;

    update();
}

