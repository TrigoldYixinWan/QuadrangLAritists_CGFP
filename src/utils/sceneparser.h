#pragma once


#include "scenedata.h"
#include <vector>
#include <string>
#include <GL/glew.h>

// Struct which contains data for a single primitive, to be used for rendering
struct RenderShapeData {
    ScenePrimitive primitive;
    glm::mat4 ctm; // the cumulative transformation matrix

    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;

    size_t indexCount = 0;
    size_t vertexCount = 0;
    bool hasIndices = false;

};

// Struct which contains all the data needed to render a scene
struct RenderData {
    SceneGlobalData globalData;
    SceneCameraData cameraData;

    std::vector<SceneLightData> lights;
    std::vector<RenderShapeData> shapes;
};

class SceneParser {
public:
    // Parse the scene and store the results in renderData.
    // @param filepath    The path of the scene file to load.
    // @param renderData  On return, this will contain the metadata of the loaded scene.
    // @return            A boolean value indicating whether the parse was successful.
    static bool parse(std::string filepath, RenderData &renderData);
    static void traverseSceneGraph(SceneNode* node, const glm::mat4& parentTransform, RenderData& renderData);
};