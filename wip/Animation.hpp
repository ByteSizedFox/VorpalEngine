#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"

#include <vector>
#include <cstdio>

struct KeyFrame {
    glm::vec3 position;
    glm::quat rotation;
    double timeStamp;
};

struct Animation {
    std::vector<KeyFrame> keyframes;
    double time;
    bool finished;
    int currentIndex;
    glm::vec3 position;
    glm::quat rotation;
};

void tickAnimation(double deltaTime, Animation *animation) {
    animation->time += deltaTime;
    int index = animation->currentIndex;
    for (int i = index; i < animation->keyframes.size()-1; i++) {
        KeyFrame keyFrame = animation->keyframes[i];
        if (animation->time >= keyFrame.timeStamp) {
            index = i;
        } else {
            break;
        }
    }
    animation->currentIndex = index;


    float timeStamp = animation->keyframes[index].timeStamp;
    float start = animation->time - timeStamp;
    KeyFrame nextKeyFrame = animation->keyframes[index + 1];
    float end = nextKeyFrame.timeStamp - timeStamp;
    float t = start / end;

    animation->position = glm::mix(animation->keyframes[index].position, nextKeyFrame.position, t);
    animation->rotation = glm::slerp(animation->keyframes[index].rotation, nextKeyFrame.rotation, t);
}

void testAnimation() {
    Animation anim;
    anim.currentIndex = 0;
    
    anim.keyframes.push_back({
        glm::vec3(0.0, 0.0, 0.0),
        glm::quat(glm::radians(glm::vec3(0.0, 0.0, 0.0))),
        0.0
    });
    anim.keyframes.push_back({
        glm::vec3(0.0, 1.0, 1.0),
        glm::quat(glm::radians(glm::vec3(0.0, 0.0, 90.0))),
        10.0
    });
    anim.keyframes.push_back({
        glm::vec3(1.0, 0.0, 0.0),
        glm::quat(glm::radians(glm::vec3(0.0, 0.0, 45.0))),
        15.0
    });

    for (int i = 0; i < 15; i++) {
        tickAnimation(1.0, &anim);
        glm::vec3 rot = glm::degrees(glm::eulerAngles(anim.rotation));
        printf("position: %f %f %f, rotation: %f %f %f\n",
            anim.position.x, anim.position.y, anim.position.z,
            rot.x, rot.y, rot.z
        );
    }
}
