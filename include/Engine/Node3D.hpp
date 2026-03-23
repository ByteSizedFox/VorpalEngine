#pragma once
#include <Engine/Vertex.hpp>
#include "Engine/Texture.hpp"

#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"

class Node3D {
protected:
    // node transform
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    glm::quat orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::mat4 modelMatrix = glm::mat4(1.0);

    // dirty update flag
    bool isDirty = false;

    bool visible = true;

public:
    Node3D() {

    }
    // setters
    void setVisible(bool v) { visible = v; }
    bool isVisible() const { return visible; }

    void setPosition(glm::vec3 position) {
        this->position = position;
        isDirty = true;
    }
    void setScale(glm::vec3 scale) {
        this->scale = scale;
        isDirty = true;
    }
    void setOrientation(glm::quat rot) {
        orientation = rot;
        isDirty = true;
    }
    void setRotation(glm::vec3 rotation) {
        orientation = glm::quat(rotation);
        isDirty = true;
    }
    void setMatrix(glm::mat4 mat) {
        modelMatrix = mat;
        // we set this to false because we are directly writing the modelMatrix which the dirty flag also does
        isDirty = false;
    }
    // getters
    glm::vec3 getPosition() {
        return position;
    }
    glm::vec3 getScale() {
        return scale;
    }
    glm::quat getOrientation() {
        return orientation;
    }
    glm::vec3 getRotation() {
        return glm::eulerAngles(orientation);
    }
    glm::mat4 getMatrix() {
        return modelMatrix;
    }
};