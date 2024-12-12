#pragma once

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include <QElapsedTimer>
#include <QOpenGLWidget>
#include <QTime>
#include <QTimer>
#include "camera.h"
#include "utils/sceneparser.h"

#include <Box2D/Box2D.h>
#include <Box2D/Particle/b2ParticleSystem.h>

// A structure for physical objects integrated with OpenGL rendering
enum class ObjectShape {
    BOX,
    CIRCLE,
    WATER
    // ... other shapes if any
};
struct PhysObject {
    b2Body* body;
    GLuint VAO;
    GLuint VBO;
    glm::vec2 size;  // half-size for box, radius for circle
    bool isCircle;
    glm::vec3 color; // Store the object color here
    ObjectShape shape;
    bool canBecomeStatic = false;
    float orbitAngularSpeed = 0.0f;
    GLuint textureID = 0;
    bool hasTexture = false;
};

class Realtime : public QOpenGLWidget {
    Q_OBJECT

public:
    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
    };

    struct DirectionalLight {
        glm::vec3 direction;
        glm::vec3 color;
    };

    struct Light {
        int type;          // 0 = Directional, 1 = Point, 2 = Spotlight
        glm::vec3 position;
        glm::vec3 direction;
        glm::vec3 color;
        float angle;       // Spotlight cutoff angle (degrees)
        float penumbra;    // Spotlight penumbra angle (degrees)
        glm::vec3 attenuation; // Attenuation (k_c, k_l, k_q)
    };

    const int MAX_LIGHTS = 8;
    std::vector<Light> lights;

    Realtime(QWidget *parent = nullptr);
    void finish();                       // Called on program exit
    void sceneChanged();
    void settingsChanged();
    void saveViewportImage(std::string filePath);



protected:
    void initializeGL() override;        // Called once at the start
    void paintGL() override;             // Called every time the view updates
    void resizeGL(int width, int height) override;

    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void timerEvent(QTimerEvent *event) override;

private:
    // Methods related to scene and rendering
    void initializeShapes();
    glm::mat4 computeModelMatrix(const std::vector<SceneTransformation>& transformations);
    void clearScene();
    void rotateAroundWorldUp(float angle);
    void rotateAroundCameraRight(float angle);
    glm::mat4 createRotationMatrix(const glm::vec3 &axis, float angle);


    void paintTexture(GLuint texture, int filterType);

    bool m_autoStaticMode = false;

    // Physics-related methods
    void stepPhysics(float dt);
    void createPhysicsObject(float x, float y);

    // Member variables
    glm::mat4 m_model = glm::mat4(1.f);
    glm::mat4 m_view  = glm::mat4(1.f);
    glm::mat4 m_proj  = glm::mat4(1.f);

    int m_timer;
    QElapsedTimer m_elapsedTimer;

    bool m_mouseDown = false;
    glm::vec2 m_prev_mouse_pos;
    std::unordered_map<Qt::Key, bool> m_keyMap;

    double m_devicePixelRatio;

    Camera m_camera;
    RenderData m_renderData;
    GLuint m_shaderProgram;
    GLuint m_shaderProgram2D;
    GLuint m_phong_shader;

    GLuint m_defaultFBO;
    int m_fbo_width;
    int m_fbo_height;
    int m_screenWidth;
    int m_screenHeight;

    GLuint m_textureShader;
    GLuint m_fullscreen_vbo;
    GLuint m_fullscreen_vao;
    QImage m_image;
    GLuint m_kitten_texture;
    GLuint m_fbo;
    GLuint m_fbo_texture;
    GLuint m_fbo_renderbuffer;

    int m_currentFilter = 0;

    // Box2D physics world and objects
    b2World* m_world = nullptr;
    float m_worldWidth;
    float m_worldHeight;
    std::vector<PhysObject> m_objects;

    void setup2DProjection(int w, int h);

    ObjectShape m_currentShape = ObjectShape::BOX;
    float m_currentSize= 0.3f; // half-size for box or radius for circle
    glm::vec3 m_currentColor = glm::vec3(0.2f, 0.2f, 0.8f);
    GLuint m_testVAO = 0;
    GLuint m_testVBO = 0;

private:
    b2Body* m_groundBody = nullptr;
    bool m_selectingGravityCenter = false;
    bool m_selectingExplosionCenter = false;
    bool m_hasGravityCenter = false;
    bool m_explosionMode = false;
    int m_explosionStrength = 10;
    glm::vec2 m_explosionCenter = glm::vec2(0.0f, 0.0f);
    glm::vec2 m_gravityCenter = glm::vec2(0.0f, 0.0f);
    float m_gravityStrength = 10.0f; // Adjust as needed

    bool m_selectingOrbitCenter = false;
    bool m_orbitMode = false;
    glm::vec2 m_orbitCenter = glm::vec2(0.0f);
    float m_orbitRadius = 2.0f; // choose a radius you like
    float m_orbitSpeed = 1.0f;  // angular velocity in rad/s
    float m_radiusTolerance = 0.1f; // tolerance for deciding if at orbit radius

    void resetGravityCenter();
    void initializeSolarSystem();

    b2ParticleSystem* m_particleSystem;
    b2ParticleSystemDef m_particleSystemDef;
    float m_particleRadius = 0.1f;
    const float m_waterDensity = 1.0f;
    void renderWaterParticles();
    void createWaterParticles(float x, float y, int count = 20);
    void drawCircle(float radius);


    // For rendering particles
    GLuint m_particleVAO = 0;
    GLuint m_particleVBO = 0;
    bool m_particleVAOInitialized = false;
    std::vector<b2Vec2> m_drawPoints;

    bool m_brushMode = false;
    b2Body* m_currentBrush = nullptr;
    float m_brushThickness = 0.1f;
    void renderBrushStrokes();
    void resetWorld();

    bool m_justFinishStroke = false;
    std::vector<b2Vec2> m_currentStroke;
    std::vector<std::vector<b2Vec2>> m_allBrushStrokes;
};

