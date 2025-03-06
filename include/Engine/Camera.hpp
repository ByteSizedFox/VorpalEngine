#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

// physics
#include "btBulletDynamicsCommon.h"
#include "LinearMath/btVector3.h"
#include "LinearMath/btAlignedObjectArray.h"

#include "config.h"

class Camera {
private:
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::quat orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // Identity quaternion
    glm::mat4 viewMatrix = glm::mat4(1.0f);
    glm::vec3 forward = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 velocity = glm::vec3(0.0,0.0,0.0);

    bool isDirty = true;

public:
    Camera() = default;

    btRigidBody* rigidBody;
    
    // Getters
    glm::vec3 getPosition() const {
        return position;
    }
    
    glm::vec3 getRotationEuler() const {
        return glm::eulerAngles(orientation);
    }
    
    glm::quat getOrientation() const {
        return orientation;
    }
    
    glm::vec3 getForward() const {
        if (isDirty) {
            const_cast<Camera*>(this)->updateVectors();
        }
        return forward;
    }
    
    glm::vec3 getRight() const {
        if (isDirty) {
            const_cast<Camera*>(this)->updateVectors();
        }
        return right;
    }
    
    glm::vec3 getUp() const {
        if (isDirty) {
            const_cast<Camera*>(this)->updateVectors();
        }
        return up;
    }
    
    // Setters
    void setPosition(const glm::vec3& newPosition) {
        position = newPosition;
        isDirty = true;
    }
    
    void setRotationEuler(const glm::vec3& eulerAngles) {
        // Create quaternion from Euler angles (pitch, yaw, roll)
        orientation = glm::quat(eulerAngles);
        orientation = glm::normalize(orientation);
        isDirty = true;
    }
    
    void setOrientation(const glm::quat& newOrientation) {
        orientation = glm::normalize(newOrientation);
        isDirty = true;
    }
    
    // Rotation methods
    void rotate(const glm::quat& rotation) {
        // Apply rotation to current orientation
        orientation = rotation * orientation;
        orientation = glm::normalize(orientation);
        isDirty = true;
    }
    
    void rotateLocal(float angle, const glm::vec3& axis) {
        // First transform the axis to local space
        glm::vec3 localAxis = glm::rotate(orientation, axis);
        // Create quaternion from angle and transformed axis
        glm::quat rotation = glm::angleAxis(angle, glm::normalize(localAxis));
        // Apply rotation
        orientation = rotation * orientation;
        orientation = glm::normalize(orientation);
        isDirty = true;
    }
    
    void rotateGlobal(float angle, const glm::vec3& axis) {
        // Create quaternion from angle and axis
        glm::quat rotation = glm::angleAxis(angle, glm::normalize(axis));
        // Apply rotation
        orientation = rotation * orientation;
        orientation = glm::normalize(orientation);
        
        isDirty = true;
    }
    
    // Specific rotation methods for common use cases
    void pitch(float angle) {  // Around local X axis
        rotateLocal(angle, glm::vec3(1.0f, 0.0f, 0.0f));
    }
    
    void yaw(float angle) {  // Around global Y axis
        rotateGlobal(angle, glm::vec3(0.0f, 1.0f, 0.0f));
    }
    
    void roll(float angle) {  // Around local Z axis
        rotateLocal(angle, glm::vec3(0.0f, 0.0f, 1.0f));
    }
    
    // Movement methods
    void move(const glm::vec3& offset) {
        position += offset;
        isDirty = true;
    }
    
    void moveForward(float distance) {
        if (isDirty) {
            updateVectors();
        }
        position += forward * distance;
        isDirty = true;
    }
    
    void moveRight(float distance) {
        if (isDirty) {
            updateVectors();
        }
        position += right * distance;
        isDirty = true;
    }
    
    void moveUp(float distance) {
        if (isDirty) {
            updateVectors();
        }
        position += up * distance;
        isDirty = true;
    }
    
    // Matrix calculations
    void updateVectors() {
        // Calculate basis vectors from orientation
        forward = glm::rotate(orientation, glm::vec3(0.0f, 0.0f, 1.0f));
        right = glm::rotate(orientation, glm::vec3(1.0f, 0.0f, 0.0f));
        up = glm::rotate(orientation, glm::vec3(0.0f, 1.0f, 0.0f));
        
        // Ensure they're normalized
        forward = glm::normalize(forward);
        right = glm::normalize(right);
        up = glm::normalize(up);
        
        isDirty = false;
    }
    
    void updateViewMatrix() {
        if (isDirty) {
            updateVectors();
        }
        
        // Create rotation matrix from orientation (use conjugate for view matrix)
        glm::mat4 rotationMatrix = glm::mat4_cast(glm::conjugate(orientation));
        
        // Create translation matrix
        glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), -position * glm::vec3(WORLD_SCALE));
        
        // Combine for view matrix
        viewMatrix = rotationMatrix * translationMatrix;
    }
    
    glm::mat4 getViewMatrix() {
        updateViewMatrix();
        return viewMatrix;
    }
    glm::vec3 getVelocity() {
        return velocity;
    }
    void setVelocity(glm::vec3 vel) {
        velocity = vel;
    }
    void resetVelocity() {
        velocity = glm::vec3(0.0);
    }

    void createRigidBody() {
        btCollisionShape* bodyShape = new btCapsuleShape(0.5, 1.0);
        btTransform bodyTransform;
        bodyTransform.setIdentity();

        float mass = 70.0;
        btVector3 localInertia(0, 0, 0);
        bodyShape->calculateLocalInertia(mass, localInertia);
        localInertia *= 0.1f;

        btDefaultMotionState* motionState = new btDefaultMotionState(bodyTransform);

        bodyShape->setMargin(0.001);

        btRigidBody* meshBody = new btRigidBody(
            btRigidBody::btRigidBodyConstructionInfo(
                mass, motionState, bodyShape, localInertia
            )
        );

        meshBody->setFriction(2.0f);
        meshBody->setRestitution(0.0f);
        meshBody->setAngularFactor(btVector3(0,0,0));

        rigidBody = meshBody;
    }
};