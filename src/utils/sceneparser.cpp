#include "sceneparser.h"
#include "scenefilereader.h"
#include <glm/gtx/transform.hpp>

#include <chrono>
#include <iostream>

bool SceneParser::parse(std::string filepath, RenderData &renderData) {
    ScenefileReader fileReader = ScenefileReader(filepath);
    bool success = fileReader.readJSON();
    if (!success) {
        return false;
    }

    // TODO: Use your Lab 5 code here

    // Task 5: populate renderData with global data, and camera data;
    renderData.globalData = fileReader.getGlobalData();
    renderData.cameraData = fileReader.getCameraData();
    // Task 6: populate renderData's list of primitives and their transforms.
    //         This will involve traversing the scene graph, and we recommend you
    //         create a helper function to do so!

    SceneNode* rootNode = fileReader.getRootNode();
    glm::mat4 identityMatrix = glm::mat4(1.0f); // Identity matrix as the initial parent transform

    traverseSceneGraph(rootNode, identityMatrix, renderData);

    return true;

}
void SceneParser::traverseSceneGraph(SceneNode* node, const glm::mat4& parentTransform, RenderData& renderData) {
    if (!node) return;

    // Start with the parent's transformation
    glm::mat4 currentTransform = parentTransform;

    // Apply this node's transformations using post-multiplication
    for (const SceneTransformation* transformation : node->transformations) {
        switch (transformation->type) {
        case TransformationType::TRANSFORMATION_TRANSLATE:
            currentTransform = currentTransform * glm::translate(glm::mat4(1.0f), transformation->translate);
            break;
        case TransformationType::TRANSFORMATION_SCALE:
            currentTransform = currentTransform * glm::scale(glm::mat4(1.0f), transformation->scale);
            break;
        case TransformationType::TRANSFORMATION_ROTATE:
            currentTransform = currentTransform * glm::rotate(glm::mat4(1.0f), transformation->angle, transformation->rotate);
            break;
        case TransformationType::TRANSFORMATION_MATRIX:
            currentTransform = currentTransform * transformation->matrix;
            break;
        }
    }
    // **Process the lights at this node**
    for (const SceneLight* light : node->lights) {
        // Create a new SceneLightData object to store the transformed light
        SceneLightData lightData;

        // Copy basic light properties
        lightData.id = light->id;
        lightData.type = light->type;
        lightData.color = light->color;
        lightData.function = light->function;
        lightData.penumbra = light->penumbra;
        lightData.angle = light->angle;
        lightData.width = light->width;
        lightData.height = light->height;

        // Handle the light based on its type
        if (light->type == LightType::LIGHT_POINT || light->type == LightType::LIGHT_SPOT) {
            // For point and spot lights, position is determined by the transformations
            glm::vec4 localPos(0.0f, 0.0f, 0.0f, 1.0f); // Origin point
            glm::vec4 transformedPos = currentTransform * localPos;
            lightData.pos = transformedPos;
        }

        if (light->type == LightType::LIGHT_DIRECTIONAL || light->type == LightType::LIGHT_SPOT) {
            // For directional and spot lights, transform the direction
            glm::vec4 localDir = glm::vec4(light->dir.x, light->dir.y, light->dir.z, 0.0f);
            glm::vec4 transformedDir = currentTransform * localDir;
            lightData.dir = glm::normalize(glm::vec4(transformedDir));
        }

        // Add the transformed light to renderData.lights
        renderData.lights.push_back(lightData);
    }

    // Process the primitives at this node
    for (const ScenePrimitive* primitive : node->primitives) {
        RenderShapeData shapeData;
        shapeData.primitive = *primitive; // Copy the primitive data
        shapeData.ctm = currentTransform; // Set the cumulative transformation matrix

        renderData.shapes.push_back(shapeData);
    }

    // Recursively traverse child nodes
    for (SceneNode* child : node->children) {
        traverseSceneGraph(child, currentTransform, renderData);
    }
}
