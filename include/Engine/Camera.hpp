#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/hash.hpp>

class Camera {
private:
    glm::vec3 position = glm::vec3(0.0,0.0,0.0);
    glm::vec2 rotation = glm::vec3(0.0,0.0,0.0);
    glm::mat4 matrix = glm::mat4(1.0);
    glm::vec3 forward = glm::vec3(0.0,0.0,0.0);
    bool isDirty = true;

public:
    glm::vec3 getPosition() {
        return position;
    }
    glm::vec2 getRotation() {
        return rotation;
    }
    void setPosition(glm::vec3 position) {
        this->position = position;
        isDirty = true;
    }
    void setRotation(glm::vec2 rotation) {
        this->rotation = rotation;
        isDirty = true;
    }
    void updateMatrix() {
        matrix = glm::mat4(1.0);
        matrix = glm::rotate(matrix, rotation.y, glm::vec3(1.0,0.0,0.0));
        matrix = glm::rotate(matrix, rotation.x, glm::vec3(0.0,1.0,0.0));

        const glm::mat4 inverted = glm::inverse(matrix);
        forward = glm::normalize(glm::vec3(inverted[2]));

        matrix = glm::translate(matrix, position);

        isDirty = false;
    }
    glm::mat4 getMatrix() {
        if (isDirty) {
            updateMatrix();
        }
        return matrix;
    }
    glm::vec3 getForward() {
        if (isDirty) {
            updateMatrix();
        }
        return forward;
    }
};